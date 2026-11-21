// Núcleo del Motor de Bloques en C++ (sin dependencias de plataforma): lee un .scbproj y ejecuta los bloques.
// Pensado para 3DS: sin hilos, sin excepciones, sin asignaciones por cuadro en el bucle principal.
#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <deque>

namespace mb {

struct Block; using BlockPtr = std::unique_ptr<Block>;
struct Slot { std::string text; BlockPtr rep; };
struct Block { std::string type; std::vector<Slot> slots; std::vector<BlockPtr> body, body2; };
struct Script { std::vector<BlockPtr> blocks; };

struct V {   // valor: número o texto
    double n = 0; std::string s; bool str = false;
    V() {} V(double x) : n(x) {} V(bool b) : n(b ? 1 : 0) {} V(const std::string& t) : s(t), str(true) {}
    double num() const;
    std::string text() const;
    bool truthy() const { return str ? (!s.empty() && s != "0" && s != "false") : n != 0; }
};

struct Obj {
    std::string name, kind = "CUBO", text, tex, say;
    double x = 0, y = 0, z = 0, w = 1, h = 1, d = 1, rot = 0, scale = 100; int color = 0xFFFFFF; bool visible = true;
    std::vector<std::unique_ptr<Script>> scripts;
    double last[3] = {0, 0, 0}; bool hasLast = false;      // última posición sin choque (bloque "chocar con")
    bool isUI() const { return kind == "BOTON" || kind == "ETIQUETA" || kind == "IMAGEN"; }
    double sw() const { return w * scale / 100; } double sh() const { return h * scale / 100; } double sd() const { return d * scale / 100; }
};

struct Scene { std::string name; int sky = 0x87CEEB; bool is2D = false; double camX = 0, camY = 0, camZ = 0, camYaw = 0, camPitch = 0; std::vector<std::unique_ptr<Obj>> objs; };

struct Input {   // la plataforma implementa esto (3DS: botones y pantalla táctil)
    virtual bool key(const std::string& name) const = 0;
    virtual bool pointerDown() const { return false; }
    virtual double pointerX() const { return 0; }
    virtual double pointerY() const { return 0; }
    virtual ~Input() {}
};

class Game {
public:
    int resW = 400, resH = 240, resW2 = 320, resH2 = 240; std::string name, console = "PC", secondName;
    std::vector<std::unique_ptr<Scene>> scenes; int cur = 0, cur2 = -1; bool quit = false;   // cur2 >= 0: hay segunda pantalla (otra escena)
    std::map<std::string, V> vars; double time = 0;
    struct Sfx { int op; std::string name; };   // op: 0 tocar, 1 música en bucle, 2 parar todo, 3 parar música
    std::vector<Sfx> sfx;                       // eventos de sonido pendientes (main.cpp los reproduce)
    std::map<std::string, std::string> texFile, sndFile;   // nombre -> archivo en romfs (TEXF|/SNDF|)

    bool load(const std::string& text);          // false si no hay escenas
    void start();                                // dispara "al empezar" y "al entrar a esta escena"
    void tick(const Input& in, double dt);       // un cuadro de lógica
    Scene& scene() { return ctx ? *ctx : *scenes[cur]; }
    Scene& sceneOf(int screen) { return *scenes[screen == 1 && cur2 >= 0 ? cur2 : cur]; }   // 0 = arriba, 1 = abajo
    bool hasSecond() const { return cur2 >= 0; }
    std::string labelText(const Obj& o) const;   // texto con {variable} ya reemplazado
    int blockCount() const;

private:
    struct Frame { const std::vector<BlockPtr>* l; size_t i = 1; int kind = 0, rem = 0; };
    struct Th { Obj* o; Script* sc; Scene* scn; int screen; std::vector<Frame> st; double wake = 0; bool dead = false; };
    std::deque<Th> threads; const Input* in = nullptr; int pendingScene = -1, pendingScreen = 0; unsigned rng = 12345;
    Scene* ctx = nullptr; int ctxScreen = 0; bool prevDown = false;

    void startHats(const std::string& type, const std::string& arg, int onlyScreen = -1);
    void startThread(Obj* o, Script* s, Scene* scn, int screen);
    void run(Th& t);
    bool exec(Th& t, Block& b);
    V ev(Th& t, const Slot& s);
    V evb(Th& t, Block& b);
    Obj* find(Obj* self, const std::string& name);
    bool overlap(const Obj& a, const Obj& b) const;
    bool hitNow(Obj* a, const std::string& name);
    void chocar(Obj* o, const std::string& name);
    V getProp(Obj& o, const std::string& p);
    void setProp(Obj& o, const std::string& p, const V& v);
    void gotoScene(int i, int screen);
    int pointerScreen() const { return cur2 >= 0 ? 1 : 0; }   // el táctil (pantalla de abajo) apunta a la segunda pantalla si existe
    double rnd();
};

} // namespace mb
