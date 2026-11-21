// Prueba en PC del núcleo (sin 3DS): g++ -std=c++17 -O2 host_test.cpp source/core.cpp -o host_test && ./host_test juego.scbproj
#include "source/core.hpp"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <set>
struct TouchKeys;
using namespace mb;

struct Keys : Input { std::set<std::string> down; bool touch = false; double tx = 0, ty = 0; bool key(const std::string& n) const override { return down.count(n) > 0; }
    bool pointerDown() const override { return touch; } double pointerX() const override { return tx; } double pointerY() const override { return ty; } };

static Obj* get(Game& g, const char* n) { for (auto& o : g.scene().objs) if (o->name == n) return o.get(); return nullptr; }
static void hold(Game& g, Keys& k, const char* key, int frames) { k.down.insert(key); for (int i = 0; i < frames; i++) g.tick(k, 1.0 / 60); k.down.erase(key); }

int main(int argc, char** argv) {
    if (argc < 3) { printf("uso: host_test <archivo.scbproj> <plat|tps|char3d|choque>\n"); return 2; }
    std::ifstream f(argv[1]); std::stringstream ss; ss << f.rdbuf();
    Game g; if (!g.load(ss.str())) { printf("no se pudo leer el proyecto\n"); return 1; }
    printf("proyecto '%s': %zu escena(s), %d bloques, resolución %dx%d\n", g.name.c_str(), g.scenes.size(), g.blockCount(), g.resW, g.resH);
    g.start(); Keys k; std::string sc = argv[2];
    if (sc == "plat") {
        Obj* p = get(g, "Jugador2D"); g.tick(k, 1.0 / 60); g.tick(k, 1.0 / 60);
        hold(g, k, "derecha", 40); printf("x=%.1f y=%.1f\n", p->x, p->y);
        k.down.insert("espacio"); for (int i = 0; i < 80; i++) { g.tick(k, 1.0 / 60); if (i == 3) k.down.erase("espacio"); }
        printf("tras saltar: y=%.1f monedas=%s\n", p->y, g.vars["monedas2d"].text().c_str());
    } else if (sc == "tps" || sc == "char3d") {
        Obj* p = get(g, "Jugador3D"); g.tick(k, 1.0 / 60); g.tick(k, 1.0 / 60);
        hold(g, k, "w", sc == "tps" ? 100 : 30); printf("z=%.2f x=%.2f\n", p->z, p->x);
        hold(g, k, "d", 20); printf("rot=%.1f x=%.2f z=%.2f\n", p->rot, p->x, p->z);
    } else if (sc == "choque") {
        Obj* a = get(g, "A"); g.tick(k, 1.0 / 60); hold(g, k, "derecha", 100); printf("A.x=%.2f (pared en x=3)\n", a->x);
        hold(g, k, "arriba", 20); printf("desliza: A.x=%.2f A.z=%.2f\n", a->x, a->z);
    }
    if (sc == "dos") {   // segunda pantalla: tocar el botón de la escena de abajo
        printf("segunda pantalla: %s (cur2=%d, %dx%d)\n", g.hasSecond() ? "sí" : "no", g.cur2, g.resW2, g.resH2);
        g.tick(k, 1.0 / 60); k.touch = true; k.tx = 100; k.ty = 110; g.tick(k, 1.0 / 60); g.tick(k, 1.0 / 60); k.touch = false; g.tick(k, 1.0 / 60); g.tick(k, 1.0 / 60);
        Obj* b = nullptr; for (auto& o : g.sceneOf(1).objs) if (o->name == "Boton") b = o.get();
        printf("clics=%s texto del botón (abajo)='%s' etiqueta arriba='%s'\n", g.vars["clics"].text().c_str(), b->text.c_str(), g.labelText(*g.sceneOf(0).objs[0]).c_str());
        k.touch = true; k.tx = 5; k.ty = 5; g.tick(k, 1.0 / 60); k.touch = false; g.tick(k, 1.0 / 60); printf("clic fuera del botón: clics=%s\n", g.vars["clics"].text().c_str());
    }
    return 0;
}
