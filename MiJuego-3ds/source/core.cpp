#include "core.hpp"
#include <cmath>
#include <cstdlib>
#include <cstdio>

namespace mb {

static const double PI = 3.14159265358979323846;
static double rad(double d) { return d * PI / 180.0; }

double V::num() const { if (!str) return n; char* e; double d = strtod(s.c_str(), &e); return e == s.c_str() ? 0 : d; }
std::string V::text() const {
    if (str) return s;
    char b[48];
    if (n == std::floor(n) && std::fabs(n) < 1e15) snprintf(b, sizeof b, "%lld", (long long)n); else snprintf(b, sizeof b, "%g", n);
    return b;
}

// ------------------------------------------------------------------ lectura del proyecto
static std::string unesc(const std::string& t) {
    std::string r; r.reserve(t.size());
    for (size_t k = 0; k < t.size(); k++) {
        if (t[k] == '%' && k + 2 < t.size() + 0 && k + 2 <= t.size() - 1) { r += (char)strtol(t.substr(k + 1, 2).c_str(), nullptr, 16); k += 2; }
        else r += t[k];
    }
    return r;
}
static std::vector<std::string> split(const std::string& s, char c, size_t maxParts = 0) {
    std::vector<std::string> out; size_t st = 0;
    while (true) {
        if (maxParts && out.size() + 1 == maxParts) { out.push_back(s.substr(st)); break; }
        size_t p = s.find(c, st);
        if (p == std::string::npos) { out.push_back(s.substr(st)); break; }
        out.push_back(s.substr(st, p - st)); st = p + 1;
    }
    return out;
}

struct Parser {
    const std::string& s; size_t i = 0;
    explicit Parser(const std::string& t) : s(t) {}
    BlockPtr block() {
        size_t st = i; while (i < s.size() && s[i] != '[') i++;
        BlockPtr b(new Block()); b->type = s.substr(st, i - st); i++;
        if (i < s.size() && s[i] == ']') i++;
        else while (i < s.size()) {
            Slot sl;
            if (s[i] == '#') { i++; size_t e = i; while (e < s.size() && s[e] != ',' && s[e] != ']') e++; sl.text = unesc(s.substr(i, e - i)); i = e; }
            else sl.rep = block();
            b->slots.push_back(std::move(sl));
            if (i < s.size() && s[i] == ',') { i++; continue; }
            i++; break;   // ']'
        }
        if (i < s.size() && s[i] == '{') {
            i++; list(b->body); i++;
            if (i < s.size() && s[i] == '{') { i++; list(b->body2); i++; }
        }
        return b;
    }
    void list(std::vector<BlockPtr>& l) { while (i < s.size() && s[i] != '}') l.push_back(block()); }
};

bool Game::load(const std::string& text) {
    Scene* sc = nullptr; Obj* cur_ = nullptr;
    size_t pos = 0;
    while (pos <= text.size()) {
        size_t e = text.find('\n', pos); if (e == std::string::npos) e = text.size();
        std::string line = text.substr(pos, e - pos); pos = e + 1;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        std::vector<std::string> f = split(line, '|');
        const std::string& k = f[0];
        if (k == "NAME" && f.size() > 1) name = unesc(f[1]);
        else if (k == "CONSOLE" && f.size() > 1) console = f[1];
        else if (k == "SECOND" && f.size() > 3) { secondName = unesc(f[1]); resW2 = atoi(f[2].c_str()); resH2 = atoi(f[3].c_str()); }
        else if (k == "RES" && f.size() > 2) { resW = atoi(f[1].c_str()); resH = atoi(f[2].c_str()); }
        else if (k == "TEXF" && f.size() > 2) texFile[unesc(f[1])] = f[2];
        else if (k == "SNDF" && f.size() > 2) sndFile[unesc(f[1])] = f[2];
        else if (k == "VAR" && f.size() > 1) vars[unesc(f[1])] = V(0.0);
        else if (k == "SCENE" && f.size() > 7) {
            scenes.emplace_back(new Scene()); sc = scenes.back().get(); cur_ = nullptr;
            sc->name = unesc(f[1]); sc->sky = atoi(f[2].c_str());
            sc->camX = atof(f[3].c_str()); sc->camY = atof(f[4].c_str()); sc->camZ = atof(f[5].c_str());
            sc->camYaw = atof(f[6].c_str()); sc->camPitch = atof(f[7].c_str()); sc->is2D = f.size() > 8 && f[8] == "1";
        } else if (k == "OBJ" && f.size() > 10) {
            if (!sc) { scenes.emplace_back(new Scene()); sc = scenes.back().get(); sc->name = "Escena 1"; }
            sc->objs.emplace_back(new Obj()); cur_ = sc->objs.back().get();
            cur_->name = unesc(f[1]); cur_->kind = f[2];
            cur_->x = atof(f[3].c_str()); cur_->y = atof(f[4].c_str()); cur_->z = atof(f[5].c_str());
            cur_->w = atof(f[6].c_str()); cur_->h = atof(f[7].c_str()); cur_->d = atof(f[8].c_str());
            cur_->rot = atof(f[9].c_str()); cur_->color = atoi(f[10].c_str());
            if (f.size() > 11) cur_->text = unesc(f[11]);
            if (f.size() > 12) cur_->tex = unesc(f[12]);
        } else if (k == "S" && cur_) {
            std::vector<std::string> g = split(line, '|', 4);
            if (g.size() < 4) continue;
            std::unique_ptr<Script> s(new Script()); Parser p(g[3]);
            while (p.i < g[3].size()) s->blocks.push_back(p.block());
            if (!s->blocks.empty()) cur_->scripts.push_back(std::move(s));
        }
    }
    if (!secondName.empty()) for (size_t i = 0; i < scenes.size(); i++) if (scenes[i]->name == secondName) cur2 = (int)i;
    return !scenes.empty();
}

static int countB(const std::vector<BlockPtr>& l) { int n = 0; for (auto& b : l) { n++; n += countB(b->body) + countB(b->body2); for (auto& s : b->slots) if (s.rep) { std::vector<BlockPtr> tmp; (void)tmp; n++; } } return n; }
int Game::blockCount() const { int n = 0; for (auto& sc : scenes) for (auto& o : sc->objs) for (auto& s : o->scripts) n += countB(s->blocks); return n; }

// ------------------------------------------------------------------ hilos
void Game::startThread(Obj* o, Script* s, Scene* scn, int screen) {
    for (auto& t : threads) if (t.o == o && t.sc == s) t.dead = true;
    Th t; t.o = o; t.sc = s; t.scn = scn; t.screen = screen; Frame f; f.l = &s->blocks; f.i = 1; t.st.push_back(f); threads.push_back(std::move(t));
}
void Game::startHats(const std::string& type, const std::string& arg, int onlyScreen) {
    for (int k = 0; k < 2; k++) {
        if ((onlyScreen >= 0 && k != onlyScreen) || (k == 1 && cur2 < 0)) continue;
        Scene* sc = &sceneOf(k);
        for (auto& o : sc->objs) for (auto& s : o->scripts) {
            Block& h = *s->blocks[0];
            if (h.type != type) continue;
            if (!arg.empty() && (h.slots.empty() || h.slots[0].text != arg)) continue;
            startThread(o.get(), s.get(), sc, k);
        }
    }
}
void Game::start() { startHats("AL_EMPEZAR", ""); startHats("AL_ESCENA", ""); }
void Game::gotoScene(int i, int screen) {
    if (i < 0 || i >= (int)scenes.size()) return;
    for (auto& t : threads) if (t.screen == screen) t.dead = true;
    if (screen == 1 && cur2 >= 0) cur2 = i; else cur = i;
    startHats("AL_ESCENA", "", screen);
}

void Game::tick(const Input& input, double dt) {
    in = &input; time += dt;
    // clic / toque sobre un objeto: "al hacer clic en este objeto" (solo en la pantalla que recibe el puntero)
    bool down = input.pointerDown();
    if (down && !prevDown) {
        int ps = pointerScreen(); Scene& sc = sceneOf(ps); double px = input.pointerX(), py = input.pointerY(); Obj* hit = nullptr;
        for (auto& o : sc.objs) if (o->visible && o->isUI() && px >= o->x && px <= o->x + o->sw() && py >= o->y && py <= o->y + o->sh()) hit = o.get();
        if (hit) for (auto& s : hit->scripts) if (s->blocks[0]->type == "AL_CLIC") startThread(hit, s.get(), &sc, ps);
    }
    prevDown = down;
    for (size_t k = 0; k < threads.size(); k++) run(threads[k]);
    for (size_t k = threads.size(); k-- > 0;) if (threads[k].dead) threads.erase(threads.begin() + k);
    if (pendingScene >= 0) { int p = pendingScene; pendingScene = -1; gotoScene(p, pendingScreen); }
}

void Game::run(Th& t) {
    if (t.dead || t.wake > time) return;
    ctx = t.scn; ctxScreen = t.screen;
    struct Reset { Game* g; ~Reset() { g->ctx = nullptr; } } reset{this};
    for (int guard = 0; !t.dead && !t.st.empty() && guard < 20000; guard++) {
        Frame* f = &t.st.back();
        if (f->i >= f->l->size()) {
            if (f->kind == 1) { f->i = 0; return; }                          // por siempre: una vuelta por cuadro
            if (f->kind == 2 || f->kind == 3) { if (f->kind == 3 || --f->rem > 0) { f->i = 0; return; } }
            t.st.pop_back(); continue;
        }
        Block& b = *(*f->l)[f->i++];
        if (exec(t, b)) return;
    }
    if (t.st.empty()) t.dead = true;
}

Obj* Game::find(Obj* self, const std::string& n) {
    if (n == "yo" || n.empty()) return self;
    for (auto& o : scene().objs) if (o->name == n) return o.get();
    return nullptr;
}

// ------------------------------------------------------------------ colisión
bool Game::overlap(const Obj& a, const Obj& b) const {
    if (a.isUI() || b.isUI()) return a.x < b.x + b.sw() && a.x + a.sw() > b.x && a.y < b.y + b.sh() && a.y + a.sh() > b.y;
    auto half = [](const Obj& o, double h[3]) {
        if (o.kind == "SPRITE") { h[0] = o.sw() / 2; h[1] = o.sh() / 2; h[2] = 0.25; }
        else { double r = std::fmax(o.sw(), o.sd()) / 2; h[0] = r; h[1] = o.sh() / 2; h[2] = r; }
    };
    double ha[3], hb[3]; half(a, ha); half(b, hb);
    return std::fabs(a.x - b.x) <= ha[0] + hb[0] && std::fabs(a.y - b.y) <= ha[1] + hb[1] && std::fabs(a.z - b.z) <= ha[2] + hb[2];
}
bool Game::hitNow(Obj* a, const std::string& name) {
    if (!a->visible) return false;
    for (auto& o : scene().objs) if (o.get() != a && o->visible && o->name == name && overlap(*a, *o)) return true;
    return false;
}
void Game::chocar(Obj* o, const std::string& name) {
    if (!o->hasLast) { o->last[0] = o->x; o->last[1] = o->y; o->last[2] = o->z; o->hasLast = true; }
    double* l = o->last;
    if (hitNow(o, name)) {
        double nx = o->x, ny = o->y, nz = o->z;
        o->x = l[0];
        if (hitNow(o, name)) { o->x = nx; o->y = l[1];
            if (hitNow(o, name)) { o->y = ny; o->z = l[2];
                if (hitNow(o, name)) { o->x = l[0]; o->y = l[1]; o->z = l[2]; } } }
    }
    if (!hitNow(o, name)) { l[0] = o->x; l[1] = o->y; l[2] = o->z; }
}

// ------------------------------------------------------------------ propiedades
V Game::getProp(Obj& o, const std::string& p) {
    if (p == "x") return V(o.x); if (p == "y") return V(o.y); if (p == "z") return V(o.z);
    if (p == "dirección") return V(o.rot); if (p == "tamaño") return V(o.scale);
    if (p == "ancho") return V(o.w); if (p == "alto") return V(o.h); if (p == "fondo") return V(o.d);
    if (p == "visible") return V(o.visible); if (p == "texto") return V(o.text);
    return V(0.0);
}
void Game::setProp(Obj& o, const std::string& p, const V& v) {
    if (p == "x") o.x = v.num(); else if (p == "y") o.y = v.num(); else if (p == "z") o.z = v.num();
    else if (p == "dirección") o.rot = v.num(); else if (p == "tamaño") o.scale = std::fmax(1, v.num());
    else if (p == "ancho") o.w = v.num(); else if (p == "alto") o.h = v.num(); else if (p == "fondo") o.d = v.num();
    else if (p == "visible") o.visible = v.truthy(); else if (p == "texto") o.text = v.text();
    else if (p == "color") o.color = (int)strtol(v.text().c_str(), nullptr, 16);
}
double Game::rnd() { rng = rng * 1664525u + 1013904223u; return (rng >> 8) / 16777216.0; }

std::string Game::labelText(const Obj& o) const {
    std::string r; const std::string& t = o.text;
    for (size_t i = 0; i < t.size(); i++) {
        if (t[i] == '{') { size_t e = t.find('}', i); if (e != std::string::npos) { auto it = vars.find(t.substr(i + 1, e - i - 1)); if (it != vars.end()) { r += it->second.text(); i = e; continue; } } }
        r += t[i];
    }
    return r;
}

// ------------------------------------------------------------------ expresiones
V Game::ev(Th& t, const Slot& s) {
    if (s.rep) return evb(t, *s.rep);
    const std::string& x = s.text;
    if (!x.empty()) { char* e; double d = strtod(x.c_str(), &e); if (e != x.c_str() && *e == 0) return V(d); }
    return V(x);
}

V Game::evb(Th& t, Block& b) {
    Obj* o = t.o; const std::string& ty = b.type; auto& s = b.slots;
    auto N = [&](size_t k) { return k < s.size() ? ev(t, s[k]).num() : 0.0; };
    auto S = [&](size_t k) { return k < s.size() ? ev(t, s[k]).text() : std::string(); };
    if (ty == "VAR") { auto it = vars.find(S(0)); return it == vars.end() ? V(0.0) : it->second; }
    if (ty == "POS_X") return V(o->x); if (ty == "POS_Y") return V(o->y); if (ty == "POS_Z") return V(o->z);
    if (ty == "DIRECCION") return V(o->rot); if (ty == "TAMANO") return V(o->scale);
    if (ty == "SUMA") return V(N(0) + N(1)); if (ty == "RESTA") return V(N(0) - N(1)); if (ty == "MULT") return V(N(0) * N(1));
    if (ty == "DIV") { double d = N(1); return V(d == 0 ? 0.0 : N(0) / d); }
    if (ty == "MOD") { double d = N(1); return V(d == 0 ? 0.0 : std::fmod(N(0), d)); }
    if (ty == "MAYOR") return V(N(0) > N(1)); if (ty == "MENOR") return V(N(0) < N(1));
    if (ty == "IGUAL") { V a = ev(t, s[0]), c = ev(t, s[1]); return V(a.str || c.str ? a.text() == c.text() : a.n == c.n); }
    if (ty == "Y") return V(ev(t, s[0]).truthy() && ev(t, s[1]).truthy());
    if (ty == "O") return V(ev(t, s[0]).truthy() || ev(t, s[1]).truthy());
    if (ty == "NO") return V(!ev(t, s[0]).truthy());
    if (ty == "UNIR") return V(S(0) + S(1)); if (ty == "LONGITUD") return V((double)S(0).size());
    if (ty == "REDONDEAR") return V(std::floor(N(0) + 0.5));
    if (ty == "ALEATORIO") { double a = N(0), c = N(1); if (c < a) std::swap(a, c); return V(a + std::floor(rnd() * (c - a + 1))); }
    if (ty == "FUNCION") {
        std::string f = S(0); double v = N(1);
        if (f == "abs") return V(std::fabs(v)); if (f == "raiz") return V(std::sqrt(std::fmax(0, v)));
        if (f == "sin") return V(std::sin(rad(v))); if (f == "cos") return V(std::cos(rad(v))); if (f == "tan") return V(std::tan(rad(v)));
        if (f == "piso") return V(std::floor(v)); return V(std::ceil(v));
    }
    if (ty == "CAM_X") return V(scene().camX); if (ty == "CAM_Y") return V(scene().camY); if (ty == "CAM_Z") return V(scene().camZ);
    if (ty == "CAM_GIRO") return V(scene().camYaw);
    if (ty == "TECLA_PRES") return V(in && in->key(S(0)));
    if (ty == "MOUSE_ABAJO") return V(in && in->pointerDown() && ctxScreen == pointerScreen());
    if (ty == "MOUSE_X") return V(in ? in->pointerX() : 0.0); if (ty == "MOUSE_Y") return V(in ? in->pointerY() : 0.0);
    if (ty == "TEMPORIZADOR") return V(time);
    if (ty == "ESCENA_NUM") return V((double)((ctxScreen == 1 && cur2 >= 0 ? cur2 : cur) + 1)); if (ty == "ESCENA_NOMBRE") return V(scene().name);
    if (ty == "PROP_DE") { Obj* g = find(o, S(1)); return g ? getProp(*g, S(0)) : V(0.0); }
    if (ty == "TOCANDO") {
        std::string n = S(0);
        if (n == "puntero") { if (!in || !in->pointerDown() || ctxScreen != pointerScreen()) return V(false); double px = in->pointerX(), py = in->pointerY(); return V(px >= o->x && px <= o->x + o->sw() && py >= o->y && py <= o->y + o->sh()); }
        return V(hitNow(o, n));
    }
    if (ty == "DISTANCIA") { Obj* g = find(o, S(0)); if (!g) return V(0.0); double dx = o->x - g->x, dy = o->y - g->y, dz = o->z - g->z; return V(std::sqrt(dx * dx + dy * dy + dz * dz)); }
    return V(0.0);   // bloques de sonido, red, Google, etc.: no existen en esta versión
}

// ------------------------------------------------------------------ sentencias (devuelve true si cede el cuadro)
bool Game::exec(Th& t, Block& b) {
    Obj* o = t.o; const std::string& ty = b.type; auto& s = b.slots;
    auto N = [&](size_t k) { return k < s.size() ? ev(t, s[k]).num() : 0.0; };
    auto S = [&](size_t k) { return k < s.size() ? ev(t, s[k]).text() : std::string(); };
    auto push = [&](const std::vector<BlockPtr>& l, int kind, int rem) { Frame f; f.l = &l; f.i = 0; f.kind = kind; f.rem = rem; t.st.push_back(f); };
    if (ty == "SIEMPRE") { push(b.body, 1, 0); return false; }
    if (ty == "REPETIR") { int c = (int)N(0); if (c > 0) push(b.body, 2, c); return false; }
    if (ty == "SI") { if (ev(t, s[0]).truthy()) push(b.body, 0, 0); return false; }
    if (ty == "SI_SINO") { push(ev(t, s[0]).truthy() ? b.body : b.body2, 0, 0); return false; }
    if (ty == "ESPERAR_HASTA") { if (!ev(t, s[0]).truthy()) { t.st.back().i--; return true; } return false; }
    if (ty == "REPETIR_HASTA") { if (!ev(t, s[0]).truthy()) { t.st.back().i--; push(b.body, 0, 0); return true; } return false; }
    if (ty == "ESPERAR") { t.wake = time + N(0); return true; }
    if (ty == "DETENER") {
        std::string w = S(0);
        if (w == "todo") { for (auto& x : threads) x.dead = true; return true; }
        if (w == "este script") { t.dead = true; return true; }
        for (auto& x : threads) if (&x != &t && x.o == o) x.dead = true;   // "otros scripts de este objeto"
        return false;
    }
    if (ty == "ENVIAR") { startHats("AL_RECIBIR", S(0)); return false; }
    if (ty == "FIJAR_VAR") { vars[S(0)] = ev(t, s[1]); return false; }
    if (ty == "CAMBIAR_VAR") { V& v = vars[S(0)]; v = V(v.num() + N(1)); return false; }
    if (ty == "MOVER") { double k = N(0), a = rad(o->rot); o->x += k * std::sin(a); o->z += k * std::cos(a); return false; }
    if (ty == "GIRAR_D") { o->rot += N(0); return false; } if (ty == "GIRAR_I") { o->rot -= N(0); return false; }
    if (ty == "APUNTAR") { o->rot = N(0); return false; }
    if (ty == "IR_A") { o->x = N(0); o->y = N(1); o->z = N(2); return false; }
    if (ty == "CAMBIAR_X") { o->x += N(0); return false; } if (ty == "CAMBIAR_Y") { o->y += N(0); return false; } if (ty == "CAMBIAR_Z") { o->z += N(0); return false; }
    if (ty == "FIJAR_X") { o->x = N(0); return false; } if (ty == "FIJAR_Y") { o->y = N(0); return false; } if (ty == "FIJAR_Z") { o->z = N(0); return false; }
    if (ty == "MOSTRAR") { o->visible = true; return false; } if (ty == "OCULTAR") { o->visible = false; return false; }
    if (ty == "TEXTO") { o->text = S(0); return false; } if (ty == "DECIR") { o->say = S(0); return false; }
    if (ty == "FIJAR_COLOR") { o->color = (int)strtol(S(0).c_str(), nullptr, 16); return false; }
    if (ty == "FIJAR_TAM") { o->scale = std::fmax(1, N(0)); return false; } if (ty == "CAMBIAR_TAM") { o->scale = std::fmax(1, o->scale + N(0)); return false; }
    if (ty == "SND_TOCAR" || ty == "SND_TOCAR_ESP") { sfx.push_back({0, S(0)}); return false; }
    if (ty == "MUSICA") { sfx.push_back({1, S(0)}); return false; }
    if (ty == "MUSICA_PARAR") { sfx.push_back({3, ""}); return false; }
    if (ty == "SND_PARAR") { sfx.push_back({2, ""}); return false; }
    if (ty == "PONER_TEXTURA") { o->tex = S(0); return false; }
    if (ty == "FIJAR_PROP") { Obj* g = find(o, S(1)); if (g) setProp(*g, S(0), ev(t, s[2])); return false; }
    if (ty == "CHOCAR") { chocar(o, S(0)); return false; }
    if (ty == "IR_ESCENA") { std::string n = S(0); for (size_t i = 0; i < scenes.size(); i++) if (scenes[i]->name == n) { pendingScene = (int)i; pendingScreen = t.screen; } return true; }
    if (ty == "SIG_ESCENA") { int c = t.screen == 1 && cur2 >= 0 ? cur2 : cur; pendingScene = (c + 1) % (int)scenes.size(); pendingScreen = t.screen; return true; }
    if (ty == "ANT_ESCENA") { int c = t.screen == 1 && cur2 >= 0 ? cur2 : cur; pendingScene = (c + (int)scenes.size() - 1) % (int)scenes.size(); pendingScreen = t.screen; return true; }
    {   // cámara 3D (misma matemática que el motor Java)
        Scene& sc = scene();
        if (ty == "CAMARA") { sc.camX = N(0); sc.camY = N(1); sc.camZ = N(2); return false; }
        if (ty == "CAM_MOVER") { sc.camX += N(0); sc.camY += N(1); sc.camZ += N(2); return false; }
        if (ty == "CAM_ADELANTE") { double k = N(0), a = rad(sc.camYaw); sc.camX += k * std::sin(a); sc.camZ += k * std::cos(a); return false; }
        if (ty == "CAM_GIRAR") { sc.camYaw += N(0); return false; }
        if (ty == "CAM_INCLINAR") { sc.camPitch = std::fmax(-89, std::fmin(89, sc.camPitch + N(0))); return false; }
        if (ty == "CAM_ORIENTAR") { sc.camYaw = N(0); sc.camPitch = std::fmax(-89, std::fmin(89, N(1))); return false; }
        if (ty == "CAM_DETRAS") {
            Obj* g = find(o, S(0)); if (!g) return false;
            double dist = N(1), hh = N(2), a = rad(g->rot);
            sc.camX = g->x - std::sin(a) * dist; sc.camY = g->y + hh; sc.camZ = g->z - std::cos(a) * dist;
            sc.camYaw = g->rot; sc.camPitch = std::atan2(hh, dist) * 180.0 / PI; return false;
        }
    }
    if (ty == "SALIR_JUEGO") { quit = true; return true; }
    return false;   // sonido, red, Google, cámara 3D...: se ignoran por ahora
}

} // namespace mb
