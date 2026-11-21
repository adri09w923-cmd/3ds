// Reproductor 3DS (libctru + citro2d). v1: escenas 2D completas (rectángulos de color, texto, botones) y
// 3D como cajas planas con perspectiva. Texturas (romfs:/tex) y sonido (ndsp, romfs:/snd). Lee romfs:/game.scbproj.
#include <3ds.h>
#include <citro2d.h>
#include <cstdio>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <cstring>
#include "core.hpp"
using namespace mb;

static std::string readFile(const char* p) {
    FILE* f = fopen(p, "rb"); if (!f) return "";
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    std::string s((size_t)n, '\0'); if (n > 0 && fread(&s[0], 1, (size_t)n, f) != (size_t)n) s.clear(); fclose(f); return s;
}
static u32 col(int rgb, u8 a = 255) { return C2D_Color32((rgb >> 16) & 255, (rgb >> 8) & 255, rgb & 255, a); }

struct Pad : Input {
    u32 held = 0; circlePosition cp{0, 0}; touchPosition tp{0, 0}; double sx = 1, sy = 1;
    void update() { hidScanInput(); held = hidKeysHeld(); hidCircleRead(&cp); if (held & KEY_TOUCH) hidTouchRead(&tp); }
    bool key(const std::string& n) const override {
        if (n == "izquierda" || n == "a") return (held & KEY_DLEFT) || cp.dx < -40;
        if (n == "derecha" || n == "d") return (held & KEY_DRIGHT) || cp.dx > 40;
        if (n == "arriba" || n == "w") return (held & KEY_DUP) || cp.dy > 40;
        if (n == "abajo" || n == "s") return (held & KEY_DDOWN) || cp.dy < -40;
        if (n == "espacio") return held & KEY_A;
        if (n == "enter") return held & KEY_START;
        if (n == "z") return held & KEY_B; if (n == "x") return held & KEY_X; if (n == "c") return held & KEY_Y;
        if (n == "q") return held & KEY_L; if (n == "e") return held & KEY_R;
        return false;
    }
    bool pointerDown() const override { return held & KEY_TOUCH; }
    double pointerX() const override { return tp.px * sx; }
    double pointerY() const override { return tp.py * sy; }
};

static C2D_TextBuf tbuf;
static void text(const std::string& s, float x, float y, float size, u32 color) {
    C2D_Text t; C2D_TextParse(&t, tbuf, s.c_str()); C2D_TextOptimize(&t);
    float k = size / 30.0f; C2D_DrawText(&t, C2D_WithColor, x, y, 0.5f, k, k, color);
}

// ---- texturas: archivo = u16 w, u16 h, RGBA8 (fila de arriba primero); w y h potencias de 2 ----
struct TexE { C3D_Tex tex; Tex3DS_SubTexture st; C2D_Image img; bool ok = false; };
static std::map<std::string, TexE*> texCache;
static TexE* getTex(Game& g, const std::string& name) {
    auto c = texCache.find(name); if (c != texCache.end()) return c->second;
    TexE* r = nullptr; auto f = g.texFile.find(name);
    if (f != g.texFile.end()) {
        std::string d = readFile(("romfs:/tex/" + f->second + ".bin").c_str());
        if (d.size() > 4) {
            int w = (u8)d[0] | ((u8)d[1] << 8), h = (u8)d[2] | ((u8)d[3] << 8);
            if (w >= 8 && h >= 8 && d.size() >= 4 + (size_t)w * h * 4) {
                r = new TexE();
                if (C3D_TexInit(&r->tex, w, h, GPU_RGBA8)) {
                    u32* dst = (u32*)r->tex.data; const u8* p = (const u8*)d.data() + 4;
                    for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
                        int gy = h - 1 - y;   // la GPU cuenta desde abajo
                        u32 m = (x & 1) | ((gy & 1) << 1) | ((x & 2) << 1) | ((gy & 2) << 2) | ((x & 4) << 2) | ((gy & 4) << 3);
                        u32 idx = ((gy >> 3) * (w >> 3) + (x >> 3)) * 64 + m;
                        const u8* q = p + ((size_t)y * w + x) * 4;
                        dst[idx] = ((u32)q[0] << 24) | ((u32)q[1] << 16) | ((u32)q[2] << 8) | q[3];
                    }
                    C3D_TexFlush(&r->tex); C3D_TexSetFilter(&r->tex, GPU_NEAREST, GPU_NEAREST);
                    r->st = {(u16)w, (u16)h, 0.0f, 1.0f, 1.0f, 0.0f}; r->img.tex = &r->tex; r->img.subtex = &r->st; r->ok = true;
                } else { delete r; r = nullptr; }
            }
        }
    }
    texCache[name] = r; return r;
}
static void drawTex(Game& g, const std::string& name, float x, float y, float w, float h, float z) {
    TexE* t = getTex(g, name); if (!t) return;
    C2D_DrawImageAt(t->img, x, y, z, nullptr, w / t->st.width, h / t->st.height);
}

// ---- sonido (ndsp): PCM16 mono 16 kHz en romfs:/snd/N.raw ----
struct Snd { s16* data = nullptr; u32 n = 0; };
static std::map<std::string, Snd> sndCache; static bool dspOk = false; static ndspWaveBuf waves[24]; static int nextCh = 1;
static Snd* getSnd(Game& g, const std::string& name) {
    auto c = sndCache.find(name); if (c != sndCache.end()) return c->second.data ? &c->second : nullptr;
    Snd s; auto f = g.sndFile.find(name);
    if (f != g.sndFile.end()) {
        std::string d = readFile(("romfs:/snd/" + f->second + ".raw").c_str());
        if (d.size() >= 2) { s.n = (u32)(d.size() / 2); s.data = (s16*)linearAlloc(s.n * 2); if (s.data) { memcpy(s.data, d.data(), s.n * 2); DSP_FlushDataCache(s.data, s.n * 2); } }
    }
    sndCache[name] = s; return s.data ? &sndCache[name] : nullptr;
}
static void playSnd(Game& g, const std::string& name, bool loop) {
    if (!dspOk) return; Snd* s = getSnd(g, name); if (!s) return;
    int ch = loop ? 0 : nextCh; if (!loop) nextCh = nextCh >= 22 ? 1 : nextCh + 1;
    ndspChnReset(ch); ndspChnSetInterp(ch, NDSP_INTERP_LINEAR); ndspChnSetRate(ch, 16000.0f); ndspChnSetFormat(ch, NDSP_FORMAT_MONO_PCM16);
    float mix[12] = {1, 1}; ndspChnSetMix(ch, mix);
    memset(&waves[ch], 0, sizeof(ndspWaveBuf)); waves[ch].data_vaddr = s->data; waves[ch].nsamples = s->n; waves[ch].looping = loop;
    ndspChnWaveBufAdd(ch, &waves[ch]);
}
static void pumpSound(Game& g) {
    for (auto& e : g.sfx) {
        if (e.op == 0) playSnd(g, e.name, false); else if (e.op == 1) playSnd(g, e.name, true);
        else if (dspOk && e.op == 3) ndspChnReset(0);
        else if (dspOk && e.op == 2) for (int i = 0; i < 24; i++) ndspChnReset(i);
    }
    g.sfx.clear();
}

static void drawScene(Game& g, Scene& sc, float k, float ox, float oy, float W, float H) {
    if (sc.is2D) {
        for (auto& o : sc.objs) {
            if (!o->visible) continue;
            float x = ox + (float)o->x * k, y = oy + (float)o->y * k;
            if (o->kind == "ETIQUETA") { text(g.labelText(*o), x, y, (float)o->h * k, col(o->color)); continue; }
            if (!o->tex.empty() && o->kind != "BOTON") drawTex(g, o->tex, x, y, (float)o->sw() * k, (float)o->sh() * k, 0.2f);
            else C2D_DrawRectSolid(x, y, 0.2f, (float)o->sw() * k, (float)o->sh() * k, col(o->color));
            if (o->kind == "BOTON") text(g.labelText(*o), x + 6 * k, y + (float)o->sh() * k * 0.2f, (float)o->sh() * k * 0.6f, col(0xFFFFFF));
        }
        return;
    }
    // 3D: cajas planas con perspectiva (misma proyección que el motor Java)
    const double yaw = sc.camYaw * M_PI / 180, pit = sc.camPitch * M_PI / 180, fl = 0.9 * H;
    const double sY = sin(yaw), cY = cos(yaw), sP = sin(pit), cP = cos(pit);
    C2D_DrawRectSolid(0, H / 2, 0.1f, W, H / 2, col(0x6DB360));   // suelo (horizonte fijo)
    struct Item { double z; float x, y, w, h; u32 c; const std::string* tex; };
    std::vector<Item> items;
    for (auto& o : sc.objs) {
        if (!o->visible || o->isUI()) continue;
        double px = o->x - sc.camX, py = o->y - sc.camY, pz = o->z - sc.camZ;
        double qx = px * cY - pz * sY, qz0 = px * sY + pz * cY;
        double qy = py * cP - qz0 * sP, qz = py * sP + qz0 * cP;
        if (qz < 0.3) continue;
        double f = fl / qz;
        items.push_back({qz, (float)(W / 2 + qx * f), (float)(H / 2 - qy * f), (float)(o->sw() * f), (float)(o->sh() * f), col(o->color), &o->tex});
    }
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z > b.z; });
    for (auto& it : items) { if (!it.tex->empty() && getTex(g, *it.tex)) drawTex(g, *it.tex, it.x - it.w / 2, it.y - it.h / 2, it.w, it.h, 0.3f); else C2D_DrawRectSolid(it.x - it.w / 2, it.y - it.h / 2, 0.3f, it.w, it.h, it.c); }
    for (auto& o : sc.objs) if (o->visible && o->kind == "ETIQUETA") text(g.labelText(*o), (float)o->x * k, (float)o->y * k, (float)o->h * k, col(o->color));
}

int main() {
    gfxInitDefault(); romfsInit(); dspOk = R_SUCCEEDED(ndspInit()); if (dspOk) ndspSetOutputMode(NDSP_OUTPUT_STEREO); C3D_Init(C3D_DEFAULT_CMDBUF_SIZE); C2D_Init(C2D_DEFAULT_MAX_OBJECTS); C2D_Prepare();
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    tbuf = C2D_TextBufNew(8192);
    Game g; std::string data = readFile("romfs:/game.scbproj");
    bool ok = !data.empty() && g.load(data);
    if (ok) { g.start(); pumpSound(g); }
    const double budgetMB = g.console == "OLD3DS" ? 64 : (g.console == "NEW3DS" ? 124 : 0);
    Pad pad; float k = ok ? std::min(400.0f / g.resW, 240.0f / g.resH) : 1;
    const bool two = ok && g.hasSecond();                                   // segunda pantalla = otra escena en la pantalla de abajo (320x240)
    float k2 = two ? std::min(320.0f / g.resW2, 240.0f / g.resH2) : 1;
    pad.sx = ok ? (two ? g.resW2 : g.resW) / 320.0 : 1; pad.sy = ok ? (two ? g.resH2 : g.resH) / 240.0 : 1;   // el táctil apunta a la escena de abajo
    float ox = ok ? (400 - g.resW * k) / 2 : 0, oy = ok ? (240 - g.resH * k) / 2 : 0;
    u64 last = osGetTime();

    while (aptMainLoop()) {
        pad.update();
        if (hidKeysDown() & KEY_SELECT) break;
        u64 now = osGetTime(); double dt = (now - last) / 1000.0; last = now; if (dt > 0.1) dt = 0.1;
        if (ok) { g.tick(pad, dt); pumpSound(g); if (g.quit) break; }
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW); C2D_TextBufClear(tbuf);
        C2D_TargetClear(top, ok ? col(g.scene().sky) : col(0x202020)); C2D_SceneBegin(top);
        if (ok) drawScene(g, g.sceneOf(0), k, ox, oy, 400, 240); else text("No se pudo leer romfs:/game.scbproj", 10, 100, 18, col(0xFF4D4D));
        C2D_TargetClear(bot, two ? col(g.sceneOf(1).sky) : col(0x15161A)); C2D_SceneBegin(bot);
        if (two) drawScene(g, g.sceneOf(1), k2, (320 - g.resW2 * k2) / 2, (240 - g.resH2 * k2) / 2, 320, 240);
        double used = osGetMemRegionUsed(MEMREGION_APPLICATION) / 1048576.0;      // RAM real usada por el juego
        char line[96]; snprintf(line, sizeof line, budgetMB > 0 ? "RAM %.1f / %.0f MB" : "RAM %.1f MB", used, budgetMB);
        text(line, 6, two ? 222 : 10, two ? 14 : 22, (budgetMB > 0 && used > budgetMB) ? col(0xFF4D4D) : col(0x7CFC7C));   // rojo si se pasa
        if (!two) text("SELECT: salir", 10, 210, 16, col(0x8A8F98));
        C3D_FrameEnd(0);
    }
    C2D_TextBufDelete(tbuf); C2D_Fini(); C3D_Fini(); if (dspOk) ndspExit(); romfsExit(); gfxExit();
    return 0;
}
