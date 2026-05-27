// ============================================================
// GENERADOR DE IMÁGENES POR CAPAS
// Estructura de Datos I 2026 - URL Quetzaltenango
// ============================================================
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <climits>

#include <functional>

#ifdef _WIN32
  #include <direct.h>
  #include <windows.h>
  #define MKDIR(p) _mkdir(p)
#else
  #include <sys/stat.h>
  #define MKDIR(p) mkdir(p, 0755)
#endif

using namespace std;

// ============================================================
// UTILIDADES
// ============================================================
string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\n\r");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\n\r");
    return s.substr(a, b - a + 1);
}

vector<string> split(const string& s, char delim) {
    vector<string> v;
    stringstream ss(s);
    string tok;
    while (getline(ss, tok, delim)) v.push_back(trim(tok));
    return v;
}

void crearDirs() {
    MKDIR("datos");
    MKDIR("graficas");
}

void correrDot(const string& dotFile, const string& pngFile) {
    string cmd = "dot -Tpng \"" + dotFile + "\" -o \"" + pngFile + "\"";
    system(cmd.c_str());
}

// ============================================================
// MATRIZ DISPERSA (lista enlazada 2D de píxeles)
// ============================================================
struct NodoPixel {
    int fila, col;
    string color;
    NodoPixel* sigFila;    // siguiente en la misma fila
    NodoPixel* sigCol;     // siguiente en la misma columna
    NodoPixel(int f, int c, const string& col)
        : fila(f), col(c), color(col), sigFila(nullptr), sigCol(nullptr) {}
};

struct CabFila {
    int fila;
    NodoPixel* primero;
    CabFila* sig;
    CabFila(int f) : fila(f), primero(nullptr), sig(nullptr) {}
};

struct CabCol {
    int col;
    NodoPixel* primero;
    CabCol* sig;
    CabCol(int c) : col(c), primero(nullptr), sig(nullptr) {}
};

class MatrizDispersa {
public:
    CabFila* filas;
    CabCol*  cols;
    int      idCapa;

    MatrizDispersa(int id) : filas(nullptr), cols(nullptr), idCapa(id) {}

    void insertar(int f, int c, const string& color) {
        NodoPixel* nodo = new NodoPixel(f, c, color);

        // --- encabezado fila ---
        CabFila* cf = filas;
        CabFila* cfPrev = nullptr;
        while (cf && cf->fila < f) { cfPrev = cf; cf = cf->sig; }
        if (!cf || cf->fila != f) {
            CabFila* nuevo = new CabFila(f);
            nuevo->sig = cf;
            if (cfPrev) cfPrev->sig = nuevo; else filas = nuevo;
            cf = nuevo;
        }
        // insertar en fila (ordenado por col)
        NodoPixel* pp = nullptr;
        NodoPixel* pc = cf->primero;
        while (pc && pc->col < c) { pp = pc; pc = pc->sigFila; }
        nodo->sigFila = pc;
        if (pp) pp->sigFila = nodo; else cf->primero = nodo;

        // --- encabezado col ---
        CabCol* cc = cols;
        CabCol* ccPrev = nullptr;
        while (cc && cc->col < c) { ccPrev = cc; cc = cc->sig; }
        if (!cc || cc->col != c) {
            CabCol* nuevo = new CabCol(c);
            nuevo->sig = cc;
            if (ccPrev) ccPrev->sig = nuevo; else cols = nuevo;
            cc = nuevo;
        }
        // insertar en col (ordenado por fila)
        NodoPixel* qp = nullptr;
        NodoPixel* qc = cc->primero;
        while (qc && qc->fila < f) { qp = qc; qc = qc->sigCol; }
        nodo->sigCol = qc;
        if (qp) qp->sigCol = nodo; else cc->primero = nodo;
    }

    // Buscar color en (f,c); retorna "" si no existe
    string getColor(int f, int c) const {
        CabFila* cf = filas;
        while (cf && cf->fila < f) cf = cf->sig;
        if (!cf || cf->fila != f) return "";
        NodoPixel* p = cf->primero;
        while (p && p->col < c) p = p->sigFila;
        if (!p || p->col != c) return "";
        return p->color;
    }

    // Dimensiones reales
    void bounds(int& minF, int& maxF, int& minC, int& maxC) const {
        minF = INT_MAX; maxF = INT_MIN;
        minC = INT_MAX; maxC = INT_MIN;
        CabFila* cf = filas;
        while (cf) {
            if (cf->fila < minF) minF = cf->fila;
            if (cf->fila > maxF) maxF = cf->fila;
            NodoPixel* p = cf->primero;
            while (p) {
                if (p->col < minC) minC = p->col;
                if (p->col > maxC) maxC = p->col;
                p = p->sigFila;
            }
            cf = cf->sig;
        }
    }

    // Graficar estructura de la matriz (dot) - estilo mejorado
    void graficarEstructura(const string& sufijo) const {
        string dotPath = "graficas/mat_" + sufijo + ".dot";
        string pngPath = "graficas/mat_" + sufijo + ".png";
        ofstream dot(dotPath.c_str());

        dot << "digraph Matriz" << idCapa << " {\n";
        dot << "  graph [bgcolor=\"#1e1e2e\" pad=0.6 splines=ortho rankdir=LR\n";
        dot << "         label=\"Matriz Dispersa  |  Capa " << idCapa << "\"\n";
        dot << "         labelloc=t fontsize=20 fontcolor=\"#cdd6f4\" fontname=\"Helvetica-Bold\"];\n";
        dot << "  node [fontname=\"Helvetica\" fontsize=11];\n";
        dot << "  edge [fontname=\"Helvetica\" fontsize=9];\n\n";

        // Nodo raiz
        dot << "  raiz [label=\"MATRIZ\\nraiz\" shape=ellipse style=filled\n";
        dot << "        fillcolor=\"#313244\" fontcolor=\"#cdd6f4\" color=\"#89b4fa\" penwidth=2];\n\n";

        // Encabezados de filas
        dot << "  subgraph cluster_filas {\n";
        dot << "    label=\"Encab. Filas\" fontcolor=\"#89dceb\" color=\"#89dceb\" style=dashed bgcolor=\"#181825\";\n";
        CabFila* cf = filas;
        while (cf) {
            dot << "    hf" << cf->fila
                << " [label=\"fila " << cf->fila << "\" shape=box style=filled"
                << " fillcolor=\"#1e66f5\" fontcolor=\"white\" color=\"#89b4fa\" penwidth=1.5];\n";
            cf = cf->sig;
        }
        dot << "  }\n\n";

        // Encabezados de columnas
        dot << "  subgraph cluster_cols {\n";
        dot << "    label=\"Encab. Cols\" fontcolor=\"#f9e2af\" color=\"#f9e2af\" style=dashed bgcolor=\"#181825\";\n";
        CabCol* cc = cols;
        while (cc) {
            dot << "    hc" << cc->col
                << " [label=\"col " << cc->col << "\" shape=box style=filled"
                << " fillcolor=\"#df8e1d\" fontcolor=\"white\" color=\"#f9e2af\" penwidth=1.5];\n";
            cc = cc->sig;
        }
        dot << "  }\n\n";

        // Nodos pixel
        dot << "  // Nodos Pixel\n";
        cf = filas;
        while (cf) {
            NodoPixel* p = cf->primero;
            while (p) {
                string nid = "n" + to_string(p->fila) + "_" + to_string(p->col);
                dot << "  " << nid
                    << " [label=\"(" << p->fila << "," << p->col << ")\\n" << p->color << "\""
                    << " shape=box style=\"filled,rounded\""
                    << " fillcolor=\"" << p->color << "\""
                    << " fontcolor=\"#000000\" color=\"#cdd6f4\" penwidth=1.5];\n";
                p = p->sigFila;
            }
            cf = cf->sig;
        }

        // Conexiones raiz
        dot << "\n";
        if (filas) dot << "  raiz -> hf" << filas->fila
                       << " [color=\"#89b4fa\" penwidth=2 label=\"  filas\" fontcolor=\"#89b4fa\"];\n";
        if (cols)  dot << "  raiz -> hc" << cols->col
                       << " [color=\"#f9e2af\" penwidth=2 label=\"  cols\" fontcolor=\"#f9e2af\"];\n";

        // Cadena encabezados fila
        cf = filas;
        while (cf && cf->sig) {
            dot << "  hf" << cf->fila << " -> hf" << cf->sig->fila
                << " [color=\"#89b4fa\" style=dashed arrowsize=0.7];\n";
            cf = cf->sig;
        }
        // Cadena encabezados col
        cc = cols;
        while (cc && cc->sig) {
            dot << "  hc" << cc->col << " -> hc" << cc->sig->col
                << " [color=\"#f9e2af\" style=dashed arrowsize=0.7];\n";
            cc = cc->sig;
        }

        // sigFila (azul claro)
        cf = filas;
        while (cf) {
            string prev = "hf" + to_string(cf->fila);
            NodoPixel* p = cf->primero;
            while (p) {
                string nid = "n" + to_string(p->fila) + "_" + to_string(p->col);
                dot << "  " << prev << " -> " << nid
                    << " [color=\"#89b4fa\" label=\"sigFila\" fontcolor=\"#89b4fa\" fontsize=8];\n";
                prev = nid;
                p = p->sigFila;
            }
            cf = cf->sig;
        }

        // sigCol (naranja)
        cc = cols;
        while (cc) {
            string prev = "hc" + to_string(cc->col);
            NodoPixel* p = cc->primero;
            while (p) {
                string nid = "n" + to_string(p->fila) + "_" + to_string(p->col);
                dot << "  " << prev << " -> " << nid
                    << " [color=\"#fab387\" label=\"sigCol\" fontcolor=\"#fab387\" fontsize=8];\n";
                prev = nid;
                p = p->sigCol;
            }
            cc = cc->sig;
        }

        dot << "}\n";
        dot.close();
        correrDot(dotPath, pngPath);
        cout << "[OK] Estructura capa " << idCapa << " -> " << pngPath << endl;
    }

    // Generar imagen visual PNG de la capa
    void generarImagenPNG(const string& sufijo) const {
        int minF, maxF, minC, maxC;
        bounds(minF, maxF, minC, maxC);
        if (minF == INT_MAX) { cout << "Capa " << idCapa << " vacia." << endl; return; }

        string dotPath = "graficas/img_" + sufijo + ".dot";
        string pngPath = "graficas/img_" + sufijo + ".png";
        ofstream dot(dotPath.c_str());

        int CELL = 40; // tamaño px por celda
        int rows = maxF - minF + 1;
        int ccols = maxC - minC + 1;
        (void)rows; (void)ccols;

        dot << "graph G {\n";
        dot << "  graph [bgcolor=white];\n";
        dot << "  node  [shape=box width=" << (CELL/72.0) << " height=" << (CELL/72.0)
            << " style=filled margin=0 label=\"\"];\n";

        for (int f = minF; f <= maxF; f++) {
            for (int c = minC; c <= maxC; c++) {
                string col = getColor(f, c);
                if (col.empty()) col = "#FFFFFF";
                string nid = "n" + to_string(f) + "_" + to_string(c);
                dot << "  " << nid << " [fillcolor=\"" << col << "\" pos=\""
                    << ((c - minC) * CELL) << "," << ((maxF - f) * CELL) << "!\"];\n";
            }
        }
        dot << "}\n";
        dot.close();

        string cmd = "neato -Tpng -n \"" + dotPath + "\" -o \"" + pngPath + "\"";
        system(cmd.c_str());
        cout << "[OK] Imagen capa " << idCapa << " -> " << pngPath << endl;
    }
};

// ============================================================
// ÁRBOL BINARIO DE BÚSQUEDA DE CAPAS
// ============================================================
struct NodoCapa {
    int id;
    MatrizDispersa* matriz;
    NodoCapa* izq;
    NodoCapa* der;
    NodoCapa(int id) : id(id), izq(nullptr), der(nullptr) {
        matriz = new MatrizDispersa(id);
    }
};

class ArbolCapas {
public:
    NodoCapa* raiz;
    ArbolCapas() : raiz(nullptr) {}

    NodoCapa* insertar(NodoCapa* nodo, int id) {
        if (!nodo) return new NodoCapa(id);
        if (id < nodo->id) nodo->izq = insertar(nodo->izq, id);
        else if (id > nodo->id) nodo->der = insertar(nodo->der, id);
        return nodo;
    }

    NodoCapa* insertar(int id) {
        raiz = insertar(raiz, id);
        return buscar(id);
    }

    NodoCapa* buscar(NodoCapa* nodo, int id) const {
        if (!nodo) return nullptr;
        if (id == nodo->id) return nodo;
        if (id < nodo->id) return buscar(nodo->izq, id);
        return buscar(nodo->der, id);
    }

    NodoCapa* buscar(int id) const { return buscar(raiz, id); }

    // Inorden -> vector
    void inorden(NodoCapa* n, vector<NodoCapa*>& v) const {
        if (!n) return;
        inorden(n->izq, v);
        v.push_back(n);
        inorden(n->der, v);
    }
    // Preorden -> vector
    void preorden(NodoCapa* n, vector<NodoCapa*>& v) const {
        if (!n) return;
        v.push_back(n);
        preorden(n->izq, v);
        preorden(n->der, v);
    }
    // Postorden -> vector
    void postorden(NodoCapa* n, vector<NodoCapa*>& v) const {
        if (!n) return;
        postorden(n->izq, v);
        postorden(n->der, v);
        v.push_back(n);
    }

    vector<NodoCapa*> recorridoInorden()   const { vector<NodoCapa*> v; inorden(raiz, v);    return v; }
    vector<NodoCapa*> recorridoPreorden()  const { vector<NodoCapa*> v; preorden(raiz, v);   return v; }
    vector<NodoCapa*> recorridoPostorden() const { vector<NodoCapa*> v; postorden(raiz, v);  return v; }

    // Graficar ABB - estilo mejorado
    void graficarDot(NodoCapa* n, ofstream& dot) const {
        if (!n) return;
        string fill = "#45475a", border = "#89b4fa";
        dot << "  c" << n->id
            << " [label=\"capa " << n->id << "\""
            << " shape=box style=\"filled,rounded\""
            << " fillcolor=\"" << fill << "\" fontcolor=\"#cdd6f4\""
            << " color=\"" << border << "\" penwidth=2];\n";
        if (n->izq) {
            dot << "  c" << n->id << " -> c" << n->izq->id
                << " [label=\"izq\" color=\"#a6e3a1\" fontcolor=\"#a6e3a1\""
                << " penwidth=1.8 fontsize=10];\n";
            graficarDot(n->izq, dot);
        }
        if (n->der) {
            dot << "  c" << n->id << " -> c" << n->der->id
                << " [label=\"der\" color=\"#f38ba8\" fontcolor=\"#f38ba8\""
                << " penwidth=1.8 fontsize=10];\n";
            graficarDot(n->der, dot);
        }
    }

    void graficar() const {
        string dotPath = "graficas/abb_capas.dot";
        string pngPath = "graficas/abb_capas.png";
        ofstream dot(dotPath.c_str());
        dot << "digraph ABBCapas {\n";
        dot << "  graph [bgcolor=\"#1e1e2e\" pad=0.6\n";
        dot << "         label=\"Arbol Binario de Busqueda  |  Capas\"\n";
        dot << "         labelloc=t fontsize=20 fontcolor=\"#cdd6f4\" fontname=\"Helvetica-Bold\"];\n";
        dot << "  node [fontname=\"Helvetica\" fontsize=12];\n";
        dot << "  edge [fontname=\"Helvetica\" fontsize=10];\n\n";
        if (!raiz) {
            dot << "  vacio [label=\"(vacío)\" shape=ellipse style=filled"
                << " fillcolor=\"#313244\" fontcolor=\"#6c7086\"];\n";
        } else {
            graficarDot(raiz, dot);
        }
        dot << "}\n";
        dot.close();
        correrDot(dotPath, pngPath);
        cout << "[OK] ABB de capas -> " << pngPath << endl;
    }
};

// ============================================================
// LISTA DE CAPAS DE UNA IMAGEN (lista simple enlazada, puntero al nodo del árbol)
// ============================================================
struct NodoListaCapa {
    NodoCapa* ref;       // apuntador al nodo real en el ABB
    NodoListaCapa* sig;
    NodoListaCapa(NodoCapa* r) : ref(r), sig(nullptr) {}
};

struct ListaCapasImg {
    NodoListaCapa* cabeza;
    ListaCapasImg() : cabeza(nullptr) {}

    void agregar(NodoCapa* ref) {
        NodoListaCapa* nuevo = new NodoListaCapa(ref);
        if (!cabeza) { cabeza = nuevo; return; }
        NodoListaCapa* p = cabeza;
        while (p->sig) p = p->sig;
        p->sig = nuevo;
    }

    vector<NodoCapa*> aVector() const {
        vector<NodoCapa*> v;
        NodoListaCapa* p = cabeza;
        while (p) { v.push_back(p->ref); p = p->sig; }
        return v;
    }
};

// ============================================================
// LISTA CIRCULAR DOBLEMENTE ENLAZADA DE IMÁGENES
// ============================================================
struct NodoImagen {
    int id;
    ListaCapasImg capas;
    NodoImagen* ant;
    NodoImagen* sig;
    NodoImagen(int id) : id(id), ant(nullptr), sig(nullptr) {}
};

class ListaCircularImagenes {
public:
    NodoImagen* cabeza;
    int tam;
    ListaCircularImagenes() : cabeza(nullptr), tam(0) {}

    bool existeId(int id) const {
        if (!cabeza) return false;
        NodoImagen* p = cabeza;
        do { if (p->id == id) return true; p = p->sig; } while (p != cabeza);
        return false;
    }

    NodoImagen* buscar(int id) const {
        if (!cabeza) return nullptr;
        NodoImagen* p = cabeza;
        do { if (p->id == id) return p; p = p->sig; } while (p != cabeza);
        return nullptr;
    }

    // Insertar ordenado por id
    NodoImagen* insertar(int id) {
        if (existeId(id)) { cout << "[!] Imagen " << id << " ya existe." << endl; return buscar(id); }
        NodoImagen* nuevo = new NodoImagen(id);
        tam++;
        if (!cabeza) {
            cabeza = nuevo;
            nuevo->sig = nuevo;
            nuevo->ant = nuevo;
            return nuevo;
        }
        // Buscar posición ordenada
        NodoImagen* p = cabeza;
        do {
            if (p->id > nuevo->id) break;
            p = p->sig;
        } while (p != cabeza);

        // Insertar antes de p
        NodoImagen* prev = p->ant;
        prev->sig = nuevo;
        nuevo->ant = prev;
        nuevo->sig = p;
        p->ant = nuevo;

        if (p == cabeza && nuevo->id < cabeza->id) cabeza = nuevo;
        return nuevo;
    }

    bool eliminar(int id) {
        NodoImagen* n = buscar(id);
        if (!n) return false;
        tam--;
        if (n->sig == n) { // único
            cabeza = nullptr;
        } else {
            n->ant->sig = n->sig;
            n->sig->ant = n->ant;
            if (cabeza == n) cabeza = n->sig;
        }
        delete n;
        return true;
    }

    void graficar() const {
        string dotPath = "graficas/lista_imagenes.dot";
        string pngPath = "graficas/lista_imagenes.png";
        ofstream dot(dotPath.c_str());
        dot << "digraph ListaImagenes {\n";
        dot << "  graph [bgcolor=\"#1e1e2e\" pad=0.6 rankdir=LR\n";
        dot << "         label=\"Lista Circular Doblemente Enlazada  |  Imagenes\"\n";
        dot << "         labelloc=t fontsize=20 fontcolor=\"#cdd6f4\" fontname=\"Helvetica-Bold\"];\n";
        dot << "  node [fontname=\"Helvetica\" fontsize=11];\n";
        dot << "  edge [fontname=\"Helvetica\" fontsize=9];\n\n";

        if (!cabeza) {
            dot << "  vacio [label=\"(lista vacia)\" shape=ellipse style=filled"
                << " fillcolor=\"#313244\" fontcolor=\"#6c7086\"];\n";
            dot << "}\n"; dot.close(); correrDot(dotPath, pngPath); return;
        }

        // Nodos imagen
        NodoImagen* p = cabeza;
        do {
            dot << "  img" << p->id
                << " [label=\"{ Imagen " << p->id << " | capas: ";
            NodoListaCapa* c = p->capas.cabeza;
            if (!c) dot << "ninguna";
            while (c) { dot << c->ref->id; if (c->sig) dot << " → "; c = c->sig; }
            dot << " }\" shape=record style=filled"
                << " fillcolor=\"#313244\" fontcolor=\"#cdd6f4\""
                << " color=\"#cba6f7\" penwidth=2];\n";
            p = p->sig;
        } while (p != cabeza);

        // Flechas sig (verde) y ant (azul punteado)
        p = cabeza;
        do {
            dot << "  img" << p->id << " -> img" << p->sig->id
                << " [label=\"sig\" color=\"#a6e3a1\" fontcolor=\"#a6e3a1\" penwidth=1.8];\n";
            dot << "  img" << p->id << " -> img" << p->ant->id
                << " [label=\"ant\" color=\"#89b4fa\" fontcolor=\"#89b4fa\""
                << " style=dashed penwidth=1.5];\n";

            // Sublista de capas con referencias al ABB
            NodoListaCapa* c = p->capas.cabeza;
            int i = 0;
            while (c) {
                string nid = "img" + to_string(p->id) + "_c" + to_string(i);
                dot << "  " << nid
                    << " [label=\"capa " << c->ref->id << "\""
                    << " shape=ellipse style=filled"
                    << " fillcolor=\"#45475a\" fontcolor=\"#f9e2af\""
                    << " color=\"#f9e2af\" penwidth=1.5];\n";
                if (i == 0)
                    dot << "  img" << p->id << " -> " << nid
                        << " [color=\"#f9e2af\" label=\"capas\" fontcolor=\"#f9e2af\"];\n";
                else {
                    string prev = "img" + to_string(p->id) + "_c" + to_string(i-1);
                    dot << "  " << prev << " -> " << nid
                        << " [color=\"#f9e2af\" arrowsize=0.7];\n";
                }
                // Referencia al nodo ABB
                dot << "  " << nid << " -> c" << c->ref->id
                    << " [style=dashed color=\"#f38ba8\" label=\"ref\""
                    << " fontcolor=\"#f38ba8\" fontsize=8];\n";
                c = c->sig; i++;
            }
            p = p->sig;
        } while (p != cabeza);

        dot << "}\n";
        dot.close();
        correrDot(dotPath, pngPath);
        cout << "[OK] Lista de imagenes -> " << pngPath << endl;
    }
};

// ============================================================
// ÁRBOL BINARIO DE BÚSQUEDA DE USUARIOS
// ============================================================
struct NodoListaImg {
    int idImagen;
    NodoListaImg* sig;
    NodoListaImg(int id) : idImagen(id), sig(nullptr) {}
};

struct NodoUsuario {
    string nombre;
    NodoListaImg* listaImagenes; // lista simple de ids de imagen
    NodoUsuario* izq;
    NodoUsuario* der;
    NodoUsuario(const string& n) : nombre(n), listaImagenes(nullptr), izq(nullptr), der(nullptr) {}

    void agregarImagen(int id) {
        NodoListaImg* nuevo = new NodoListaImg(id);
        if (!listaImagenes) { listaImagenes = nuevo; return; }
        NodoListaImg* p = listaImagenes;
        while (p->sig) p = p->sig;
        p->sig = nuevo;
    }

    bool tieneImagen(int id) const {
        NodoListaImg* p = listaImagenes;
        while (p) { if (p->idImagen == id) return true; p = p->sig; }
        return false;
    }

    void eliminarImagen(int id) {
        NodoListaImg* p = listaImagenes, *prev = nullptr;
        while (p) {
            if (p->idImagen == id) {
                if (prev) prev->sig = p->sig; else listaImagenes = p->sig;
                delete p; return;
            }
            prev = p; p = p->sig;
        }
    }
};

class ArbolUsuarios {
public:
    NodoUsuario* raiz;
    ArbolUsuarios() : raiz(nullptr) {}

    NodoUsuario* insertar(NodoUsuario* n, const string& nombre) {
        if (!n) return new NodoUsuario(nombre);
        if (nombre < n->nombre) n->izq = insertar(n->izq, nombre);
        else if (nombre > n->nombre) n->der = insertar(n->der, nombre);
        return n;
    }

    NodoUsuario* insertar(const string& nombre) {
        raiz = insertar(raiz, nombre);
        return buscar(nombre);
    }

    NodoUsuario* buscar(NodoUsuario* n, const string& nombre) const {
        if (!n) return nullptr;
        if (nombre == n->nombre) return n;
        if (nombre < n->nombre) return buscar(n->izq, nombre);
        return buscar(n->der, nombre);
    }

    NodoUsuario* buscar(const string& nombre) const { return buscar(raiz, nombre); }

    NodoUsuario* minimo(NodoUsuario* n) const {
        while (n->izq) n = n->izq;
        return n;
    }

    NodoUsuario* eliminar(NodoUsuario* n, const string& nombre) {
        if (!n) return nullptr;
        if (nombre < n->nombre) n->izq = eliminar(n->izq, nombre);
        else if (nombre > n->nombre) n->der = eliminar(n->der, nombre);
        else {
            if (!n->izq) { NodoUsuario* t = n->der; delete n; return t; }
            if (!n->der) { NodoUsuario* t = n->izq; delete n; return t; }
            NodoUsuario* suc = minimo(n->der);
            n->nombre = suc->nombre;
            n->listaImagenes = suc->listaImagenes;
            suc->listaImagenes = nullptr;
            n->der = eliminar(n->der, suc->nombre);
        }
        return n;
    }

    bool eliminar(const string& nombre) {
        if (!buscar(nombre)) return false;
        raiz = eliminar(raiz, nombre);
        return true;
    }

    void graficarDot(NodoUsuario* n, ofstream& dot) const {
        if (!n) return;
        dot << "  u_" << n->nombre
            << " [label=\"{ " << n->nombre << " | imgs: ";
        NodoListaImg* p = n->listaImagenes;
        if (!p) dot << "ninguna";
        while (p) { dot << p->idImagen; if (p->sig) dot << "→"; p = p->sig; }
        dot << " }\" shape=record style=filled"
            << " fillcolor=\"#313244\" fontcolor=\"#cdd6f4\""
            << " color=\"#89dceb\" penwidth=2];\n";

        if (n->izq) {
            dot << "  u_" << n->nombre << " -> u_" << n->izq->nombre
                << " [label=\"izq\" color=\"#a6e3a1\" fontcolor=\"#a6e3a1\" penwidth=1.8 fontsize=10];\n";
            graficarDot(n->izq, dot);
        }
        if (n->der) {
            dot << "  u_" << n->nombre << " -> u_" << n->der->nombre
                << " [label=\"der\" color=\"#f38ba8\" fontcolor=\"#f38ba8\" penwidth=1.8 fontsize=10];\n";
            graficarDot(n->der, dot);
        }
    }

    void graficar() const {
        string dotPath = "graficas/abb_usuarios.dot";
        string pngPath = "graficas/abb_usuarios.png";
        ofstream dot(dotPath.c_str());
        dot << "digraph ABBUsuarios {\n";
        dot << "  graph [bgcolor=\"#1e1e2e\" pad=0.6\n";
        dot << "         label=\"Arbol Binario de Busqueda  |  Usuarios\"\n";
        dot << "         labelloc=t fontsize=20 fontcolor=\"#cdd6f4\" fontname=\"Helvetica-Bold\"];\n";
        dot << "  node [fontname=\"Helvetica\" fontsize=12];\n";
        dot << "  edge [fontname=\"Helvetica\" fontsize=10];\n\n";
        if (!raiz) {
            dot << "  vacio [label=\"(vacío)\" shape=ellipse style=filled"
                << " fillcolor=\"#313244\" fontcolor=\"#6c7086\"];\n";
        } else {
            graficarDot(raiz, dot);
        }
        dot << "}\n";
        dot.close();
        correrDot(dotPath, pngPath);
        cout << "[OK] ABB de usuarios -> " << pngPath << endl;
    }
};

// ============================================================
// GENERACIÓN DE IMÁGENES COMPUESTAS (superponer capas)
// ============================================================

// Combina N capas y genera una imagen PNG
void generarImagenCompuesta(const vector<NodoCapa*>& capas, const string& sufijo) {
    if (capas.empty()) { cout << "[!] Sin capas para generar imagen." << endl; return; }

    // Encontrar dimensiones globales
    int minF = INT_MAX, maxF = INT_MIN, minC = INT_MAX, maxC = INT_MIN;
    for (auto* c : capas) {
        int f1, f2, c1, c2;
        c->matriz->bounds(f1, f2, c1, c2);
        if (f1 < minF) minF = f1;
        if (f2 > maxF) maxF = f2;
        if (c1 < minC) minC = c1;
        if (c2 > maxC) maxC = c2;
    }
    if (minF == INT_MAX) { cout << "[!] Todas las capas están vacías." << endl; return; }

    int CELL = 40;
    string dotPath = "graficas/" + sufijo + ".dot";
    string pngPath = "graficas/" + sufijo + ".png";
    ofstream dot(dotPath.c_str());
    dot << "graph G {\n  graph [bgcolor=white];\n";
    dot << "  node [shape=box width=" << (CELL/72.0) << " height=" << (CELL/72.0)
        << " style=filled margin=0 label=\"\"];\n";

    for (int f = minF; f <= maxF; f++) {
        for (int c = minC; c <= maxC; c++) {
            string color = "#FFFFFF";
            // Aplicar capas en orden (última capa "pinta encima")
            for (auto* capa : capas) {
                string col = capa->matriz->getColor(f, c);
                if (!col.empty()) color = col;
            }
            string nid = "n" + to_string(f) + "_" + to_string(c);
            dot << "  " << nid << " [fillcolor=\"" << color << "\" pos=\""
                << ((c - minC) * CELL) << "," << ((maxF - f) * CELL) << "!\"];\n";
        }
    }
    dot << "}\n";
    dot.close();

    string cmd = "neato -Tpng -n \"" + dotPath + "\" -o \"" + pngPath + "\"";
    system(cmd.c_str());
    cout << "[OK] Imagen generada: " << pngPath << endl;
}

// Graficar imagen + árbol de capas con enlace (ilustración 7) - estilo mejorado
void graficarImagenYArbol(NodoImagen* img, const ArbolCapas& arbol) {
    string sufijo = "img_arbol_" + to_string(img->id);
    string dotPath = "graficas/" + sufijo + ".dot";
    string pngPath = "graficas/" + sufijo + ".png";
    ofstream dot(dotPath.c_str());

    dot << "digraph ImgArbol" << img->id << " {\n";
    dot << "  graph [bgcolor=\"#1e1e2e\" pad=0.6\n";
    dot << "         label=\"Imagen " << img->id << "  |  Lista de Capas y ABB\"\n";
    dot << "         labelloc=t fontsize=20 fontcolor=\"#cdd6f4\" fontname=\"Helvetica-Bold\"];\n";
    dot << "  node [fontname=\"Helvetica\" fontsize=11];\n";
    dot << "  edge [fontname=\"Helvetica\" fontsize=9];\n\n";

    // Nodo imagen
    dot << "  img" << img->id
        << " [label=\"Imagen " << img->id << "\""
        << " shape=box style=\"filled,rounded\""
        << " fillcolor=\"#40a02b\" fontcolor=\"white\""
        << " color=\"#a6e3a1\" penwidth=2];\n\n";

    // Lista de capas
    NodoListaCapa* p = img->capas.cabeza;
    int i = 0;
    while (p) {
        string nid = "lc" + to_string(i);
        dot << "  " << nid
            << " [label=\"capa " << p->ref->id << "\""
            << " shape=ellipse style=filled"
            << " fillcolor=\"#45475a\" fontcolor=\"#f9e2af\""
            << " color=\"#f9e2af\" penwidth=1.5];\n";
        if (i == 0)
            dot << "  img" << img->id << " -> lc0"
                << " [color=\"#f9e2af\" label=\"lista capas\" fontcolor=\"#f9e2af\" penwidth=2];\n";
        else {
            string prev = "lc" + to_string(i-1);
            dot << "  " << prev << " -> " << nid
                << " [color=\"#f9e2af\" arrowsize=0.8];\n";
        }
        // Referencia al ABB
        dot << "  " << nid << " -> c" << p->ref->id
            << " [style=dashed color=\"#f38ba8\" label=\"ref\""
            << " fontcolor=\"#f38ba8\" penwidth=1.5];\n";
        p = p->sig; i++;
    }

    // ABB completo
    function<void(NodoCapa*)> dotArbol = [&](NodoCapa* n) {
        if (!n) return;
        dot << "  c" << n->id
            << " [label=\"capa " << n->id << "\""
            << " shape=box style=\"filled,rounded\""
            << " fillcolor=\"#313244\" fontcolor=\"#cdd6f4\""
            << " color=\"#89b4fa\" penwidth=2];\n";
        if (n->izq) {
            dot << "  c" << n->id << " -> c" << n->izq->id
                << " [label=\"izq\" color=\"#a6e3a1\" fontcolor=\"#a6e3a1\" penwidth=1.5];\n";
            dotArbol(n->izq);
        }
        if (n->der) {
            dot << "  c" << n->id << " -> c" << n->der->id
                << " [label=\"der\" color=\"#f38ba8\" fontcolor=\"#f38ba8\" penwidth=1.5];\n";
            dotArbol(n->der);
        }
    };
    dotArbol(arbol.raiz);

    dot << "}\n";
    dot.close();
    correrDot(dotPath, pngPath);
    cout << "[OK] Imagen+Arbol -> " << pngPath << endl;
}

// ============================================================
// CARGA MASIVA
// ============================================================
void cargaMasivaCapas(const string& archivo, ArbolCapas& arbol) {
    ifstream f(archivo.c_str());
    if (!f.is_open()) { cout << "[!] No se pudo abrir: " << archivo << endl; return; }
    string linea;
    int cnt = 0;
    string bloque = "";
    int idActual = -1;

    while (getline(f, linea)) {
        linea = trim(linea);
        if (linea.empty()) continue;

        // Detectar inicio de capa: "id {"
        if (linea.find('{') != string::npos) {
            // Puede ser "id {" en la misma linea
            size_t pos = linea.find('{');
            string sId = trim(linea.substr(0, pos));
            try { idActual = stoi(sId); } catch(...) { continue; }

            NodoCapa* capa = arbol.insertar(idActual);
            if (!capa) continue;

            // Acumular el resto de la línea hasta '}'
            string resto = linea.substr(pos + 1);
            // Procesar píxeles inline o en múltiples líneas
            bool cerrado = false;
            string acum = resto;
            // buscar cierre
            if (acum.find('}') != string::npos) {
                acum = acum.substr(0, acum.find('}'));
                cerrado = true;
            }
            // procesar pixels
            auto procPixels = [&](const string& data) {
                vector<string> pixels = split(data, ';');
                for (auto& px : pixels) {
                    if (px.empty()) continue;
                    vector<string> partes = split(px, ',');
                    if (partes.size() >= 3) {
                        try {
                            int fi = stoi(partes[0]);
                            int co = stoi(partes[1]);
                            string color = trim(partes[2]);
                            if (color.empty() || color[0] != '#') color = "#FF0000";
                            capa->matriz->insertar(fi, co, color);
                        } catch(...) {}
                    }
                }
            };
            if (!cerrado) {
                // leer más líneas hasta '}'
                while (getline(f, linea)) {
                    linea = trim(linea);
                    if (linea.find('}') != string::npos) {
                        acum += linea.substr(0, linea.find('}'));
                        cerrado = true; break;
                    }
                    acum += linea;
                }
            }
            procPixels(acum);
            cout << "[OK] Capa " << idActual << " cargada." << endl;
            cnt++;
        }
    }
    cout << cnt << " capas cargadas exitosamente." << endl;
}

void cargaMasivaImagenes(const string& archivo, ListaCircularImagenes& lista, ArbolCapas& arbol) {
    ifstream f(archivo.c_str());
    if (!f.is_open()) { cout << "[!] No se pudo abrir: " << archivo << endl; return; }
    string linea;
    int cnt = 0;
    while (getline(f, linea)) {
        linea = trim(linea);
        if (linea.empty()) continue;
        size_t lb = linea.find('{');
        size_t rb = linea.find('}');
        if (lb == string::npos || rb == string::npos) continue;
        try {
            int id = stoi(linea.substr(0, lb));
            NodoImagen* img = lista.insertar(id);
            string interior = linea.substr(lb + 1, rb - lb - 1);
            if (!interior.empty()) {
                vector<string> ids = split(interior, ',');
                for (auto& sid : ids) {
                    if (sid.empty()) continue;
                    try {
                        int idCapa = stoi(sid);
                        NodoCapa* c = arbol.buscar(idCapa);
                        if (c) img->capas.agregar(c);
                        else cout << "[!] Capa " << idCapa << " no encontrada para imagen " << id << endl;
                    } catch(...) {}
                }
            }
            cout << "[OK] Imagen " << id << " cargada." << endl;
            cnt++;
        } catch(...) {}
    }
    cout << cnt << " imágenes cargadas exitosamente." << endl;
}

void cargaMasivaUsuarios(const string& archivo, ArbolUsuarios& arbolU, ListaCircularImagenes& listaImg) {
    ifstream f(archivo.c_str());
    if (!f.is_open()) { cout << "[!] No se pudo abrir: " << archivo << endl; return; }
    string linea;
    int cnt = 0;
    while (getline(f, linea)) {
        linea = trim(linea);
        if (linea.empty()) continue;
        size_t p1 = linea.find(':');
        size_t p2 = linea.find(';');
        if (p1 == string::npos) continue;
        string nombre = trim(linea.substr(0, p1));
        NodoUsuario* u = arbolU.insertar(nombre);
        string imgs = "";
        if (p2 != string::npos && p2 > p1 + 1)
            imgs = trim(linea.substr(p1 + 1, p2 - p1 - 1));
        if (!imgs.empty()) {
            vector<string> ids = split(imgs, ',');
            for (auto& sid : ids) {
                if (sid.empty()) continue;
                try {
                    int idImg = stoi(sid);
                    if (listaImg.existeId(idImg)) u->agregarImagen(idImg);
                    else cout << "[!] Imagen " << idImg << " no existe para usuario " << nombre << endl;
                } catch(...) {}
            }
        }
        cout << "[OK] Usuario " << nombre << " cargado." << endl;
        cnt++;
    }
    cout << cnt << " usuarios cargados exitosamente." << endl;
}

// ============================================================
// ARCHIVOS DE EJEMPLO
// ============================================================
void crearArchivoEjemplo() {
    // capas
    ofstream cap("datos/capas.cap");
    cap << "1 {\n";
    cap << "2,3,#FF0000;\n2,4,#FF0000;\n3,2,#FF0000;\n3,3,#FF0000;\n3,4,#FF0000;\n3,5,#FF0000;\n";
    cap << "4,2,#FF0000;\n4,3,#FF0000;\n4,4,#FF0000;\n4,5,#FF0000;\n";
    cap << "5,3,#FF0000;\n5,4,#FF0000;\n6,4,#FF0000;\n";
    cap << "}\n";
    cap << "2 {\n";
    cap << "1,1,#00AA00;\n1,2,#00AA00;\n1,3,#00AA00;\n2,1,#00AA00;\n2,3,#00AA00;\n3,1,#00AA00;\n3,2,#00AA00;\n3,3,#00AA00;\n";
    cap << "}\n";
    cap << "3 {\n";
    cap << "0,0,#0000FF;\n0,1,#0000FF;\n1,0,#0000FF;\n1,1,#0000FF;\n";
    cap << "}\n";
    cap << "4 {\n";
    cap << "2,2,#8B4513;\n2,3,#8B4513;\n3,2,#8B4513;\n3,3,#8B4513;\n";
    cap << "}\n";
    cap << "5 {\n";
    cap << "1,4,#FF6600;\n1,5,#FF6600;\n2,4,#FF6600;\n2,5,#FF6600;\n";
    cap << "}\n";
    cap.close();

    // imágenes
    ofstream im("datos/imagenes.im");
    im << "1{2,3,4}\n";
    im << "2{4,1}\n";
    im << "3{}\n";
    im << "4{1,2,3,4,5}\n";
    im << "5{3,5}\n";
    im.close();

    // usuarios
    ofstream usr("datos/usuarios.usr");
    usr << "userM:;\n";
    usr << "userB:;\n";
    usr << "userA:1,2;\n";
    usr << "userY:4,5;\n";
    usr << "userZ:3;\n";
    usr.close();

    cout << "[OK] Archivos de ejemplo creados en datos/." << endl;
}

// ============================================================
// MENÚS
// ============================================================
void pausar() {
    cout << "\nPresione Enter para continuar...";
    cin.ignore();
    cin.get();
}

void menuGeneracion(ArbolCapas& arbol, ListaCircularImagenes& listaImg) {
    int op;
    do {
        cout << "\n============================================\n";
        cout << "  GENERACION DE IMAGENES\n";
        cout << "============================================\n";
        cout << "[1] Por recorrido limitado\n";
        cout << "[2] Por lista (id imagen)\n";
        cout << "[3] Por capa\n";
        cout << "[4] Por usuario\n";
        cout << "[0] Volver\n";
        cout << "Opcion: "; cin >> op;

        if (op == 1) {
            int numCapas, tipoRec;
            cout << "Numero de capas: "; cin >> numCapas;
            cout << "Recorrido: [1]Inorden [2]Preorden [3]Postorden\nOpcion: "; cin >> tipoRec;

            vector<NodoCapa*> todas;
            if (tipoRec == 1) todas = arbol.recorridoInorden();
            else if (tipoRec == 2) todas = arbol.recorridoPreorden();
            else todas = arbol.recorridoPostorden();

            if ((int)todas.size() < numCapas) numCapas = (int)todas.size();
            vector<NodoCapa*> sel(todas.begin(), todas.begin() + numCapas);
            generarImagenCompuesta(sel, "gen_recorrido");

        } else if (op == 2) {
            int idImg;
            cout << "ID de imagen: "; cin >> idImg;
            NodoImagen* img = listaImg.buscar(idImg);
            if (!img) { cout << "[!] Imagen no encontrada." << endl; }
            else {
                vector<NodoCapa*> capas = img->capas.aVector();
                generarImagenCompuesta(capas, "gen_img_" + to_string(idImg));
            }

        } else if (op == 3) {
            int idCapa;
            cout << "ID de capa: "; cin >> idCapa;
            NodoCapa* c = arbol.buscar(idCapa);
            if (!c) { cout << "[!] Capa no encontrada." << endl; }
            else {
                c->matriz->generarImagenPNG("capa_" + to_string(idCapa));
                c->matriz->graficarEstructura(to_string(idCapa));
            }

        } else if (op == 4) {
            string nombre;
            cout << "Nombre de usuario: "; cin >> nombre;
            // buscar usuario en árbol
            // necesitamos acceso al árbol de usuarios - pasarlo por referencia
            cout << "[!] Use el menu de Usuarios para ver imágenes de un usuario." << endl;
        }
    } while (op != 0);
}

void menuUsuarios(ArbolUsuarios& arbolU, ListaCircularImagenes& listaImg, ArbolCapas& arbolC) {
    int op;
    do {
        cout << "\n============================================\n";
        cout << "  USUARIOS\n";
        cout << "============================================\n";
        cout << "[1] Agregar usuario\n";
        cout << "[2] Eliminar usuario\n";
        cout << "[3] Modificar usuario\n";
        cout << "[4] Generar imagen de usuario\n";
        cout << "[0] Volver\n";
        cout << "Opcion: "; cin >> op;

        if (op == 1) {
            string nombre;
            cout << "Nombre: "; cin >> nombre;
            if (arbolU.buscar(nombre)) cout << "[!] Usuario ya existe." << endl;
            else { arbolU.insertar(nombre); cout << "[OK] Usuario " << nombre << " agregado." << endl; }

        } else if (op == 2) {
            string nombre;
            cout << "Nombre: "; cin >> nombre;
            if (arbolU.eliminar(nombre)) cout << "[OK] Usuario eliminado." << endl;
            else cout << "[!] Usuario no encontrado." << endl;

        } else if (op == 3) {
            string nombre;
            cout << "Nombre actual: "; cin >> nombre;
            NodoUsuario* u = arbolU.buscar(nombre);
            if (!u) { cout << "[!] No encontrado." << endl; continue; }
            cout << "[1] Agregar imagen  [2] Eliminar imagen\nOpcion: ";
            int sub; cin >> sub;
            if (sub == 1) {
                int idImg; cout << "ID imagen: "; cin >> idImg;
                if (!listaImg.existeId(idImg)) cout << "[!] Imagen no existe en el sistema." << endl;
                else if (u->tieneImagen(idImg)) cout << "[!] Usuario ya tiene esa imagen." << endl;
                else { u->agregarImagen(idImg); cout << "[OK] Imagen agregada." << endl; }
            } else {
                int idImg; cout << "ID imagen: "; cin >> idImg;
                u->eliminarImagen(idImg);
                cout << "[OK] Imagen eliminada del usuario." << endl;
            }

        } else if (op == 4) {
            string nombre;
            cout << "Nombre: "; cin >> nombre;
            NodoUsuario* u = arbolU.buscar(nombre);
            if (!u) { cout << "[!] No encontrado." << endl; continue; }
            cout << "Imágenes del usuario: ";
            NodoListaImg* p = u->listaImagenes;
            if (!p) { cout << "(ninguna)" << endl; continue; }
            while (p) { cout << p->idImagen << " "; p = p->sig; }
            cout << endl;
            int idImg; cout << "Seleccione ID imagen: "; cin >> idImg;
            NodoImagen* img = listaImg.buscar(idImg);
            if (!img) { cout << "[!] Imagen no encontrada." << endl; continue; }
            if (!u->tieneImagen(idImg)) { cout << "[!] Esa imagen no pertenece al usuario." << endl; continue; }
            vector<NodoCapa*> capas = img->capas.aVector();
            generarImagenCompuesta(capas, "gen_usr_" + nombre + "_img_" + to_string(idImg));
        }
    } while (op != 0);
}

void menuImagenes(ListaCircularImagenes& listaImg, ArbolCapas& arbolC, ArbolUsuarios& arbolU) {
    int op;
    do {
        cout << "\n============================================\n";
        cout << "  IMAGENES\n";
        cout << "============================================\n";
        cout << "[1] Agregar imagen\n";
        cout << "[2] Eliminar imagen\n";
        cout << "[0] Volver\n";
        cout << "Opcion: "; cin >> op;

        if (op == 1) {
            int idImg; cout << "ID imagen: "; cin >> idImg;
            if (listaImg.existeId(idImg)) { cout << "[!] ID ya existe." << endl; continue; }
            string nombre; cout << "Usuario propietario: "; cin >> nombre;
            NodoUsuario* u = arbolU.buscar(nombre);
            if (!u) { cout << "[!] Usuario no encontrado." << endl; continue; }
            NodoImagen* img = listaImg.insertar(idImg);
            u->agregarImagen(idImg);
            cout << "Agregar capas (0 para terminar):\n";
            int idCapa;
            while (cin >> idCapa && idCapa != 0) {
                NodoCapa* c = arbolC.buscar(idCapa);
                if (c) { img->capas.agregar(c); cout << "[OK] Capa " << idCapa << " agregada." << endl; }
                else cout << "[!] Capa no encontrada." << endl;
            }
            cout << "[OK] Imagen " << idImg << " creada." << endl;

        } else if (op == 2) {
            int idImg; cout << "ID imagen: "; cin >> idImg;
            // Eliminar de lista de usuarios
            // (recorremos árbol de usuarios en inorden)
            function<void(NodoUsuario*)> limpiar = [&](NodoUsuario* n) {
                if (!n) return;
                n->eliminarImagen(idImg);
                limpiar(n->izq);
                limpiar(n->der);
            };
            limpiar(arbolU.raiz);
            if (listaImg.eliminar(idImg)) cout << "[OK] Imagen " << idImg << " eliminada." << endl;
            else cout << "[!] Imagen no encontrada." << endl;
        }
    } while (op != 0);
}

void menuEstadoMemoria(ArbolCapas& arbolC, ListaCircularImagenes& listaImg, ArbolUsuarios& arbolU) {
    int op;
    do {
        cout << "\n============================================\n";
        cout << "  ESTADO DE LA MEMORIA (GRAFICAS)\n";
        cout << "============================================\n";
        cout << "[1] Ver lista de imagenes\n";
        cout << "[2] Ver arbol de capas\n";
        cout << "[3] Ver capa\n";
        cout << "[4] Ver imagen y arbol de capas\n";
        cout << "[5] Ver arbol de usuarios\n";
        cout << "[0] Volver\n";
        cout << "Opcion: "; cin >> op;

        if (op == 1) {
            listaImg.graficar();
        } else if (op == 2) {
            arbolC.graficar();
        } else if (op == 3) {
            int id; cout << "ID capa: "; cin >> id;
            NodoCapa* c = arbolC.buscar(id);
            if (!c) cout << "[!] Capa no encontrada." << endl;
            else c->matriz->graficarEstructura(to_string(id));
        } else if (op == 4) {
            int id; cout << "ID imagen: "; cin >> id;
            NodoImagen* img = listaImg.buscar(id);
            if (!img) cout << "[!] Imagen no encontrada." << endl;
            else graficarImagenYArbol(img, arbolC);
        } else if (op == 5) {
            arbolU.graficar();
        }
    } while (op != 0);
}

// ============================================================
// MAIN
// ============================================================
int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    crearDirs();

    // Verificar/crear archivos de ejemplo
    {
        ifstream t("datos/capas.cap");
        if (!t.good()) crearArchivoEjemplo();
    }

    ArbolCapas          arbolCapas;
    ListaCircularImagenes listaImagenes;
    ArbolUsuarios       arbolUsuarios;

    cout << "============================================\n";
    cout << "  Generador de Imagenes por Capas\n";
    cout << "  Estructura de Datos I 2026 - URL Xela\n";
    cout << "  Archivos de datos en: datos/\n";
    cout << "  Graficas generadas en: graficas/\n";
    cout << "============================================\n";

    int opcion;
    do {
        cout << "\n========================================\n";
        cout << "  MENU PRINCIPAL\n";
        cout << "========================================\n";
        cout << "1. Carga Masiva\n";
        cout << "2. Generacion de Imagenes\n";
        cout << "3. Usuarios\n";
        cout << "4. Imagenes\n";
        cout << "5. Estado de la Memoria (Graficas)\n";
        cout << "0. Salir\n";
        cout << "Opcion: ";
        cin >> opcion;

        switch (opcion) {
            case 1: {
                cout << "\n--- CARGA MASIVA ---\n";
                cout << "Cargando capas...    (datos/capas.cap)\n";
                cargaMasivaCapas("datos/capas.cap", arbolCapas);
                cout << "\nCargando imagenes... (datos/imagenes.im)\n";
                cargaMasivaImagenes("datos/imagenes.im", listaImagenes, arbolCapas);
                cout << "\nCargando usuarios... (datos/usuarios.usr)\n";
                cargaMasivaUsuarios("datos/usuarios.usr", arbolUsuarios, listaImagenes);
                break;
            }
            case 2:
                menuGeneracion(arbolCapas, listaImagenes);
                break;
            case 3:
                menuUsuarios(arbolUsuarios, listaImagenes, arbolCapas);
                break;
            case 4:
                menuImagenes(listaImagenes, arbolCapas, arbolUsuarios);
                break;
            case 5:
                menuEstadoMemoria(arbolCapas, listaImagenes, arbolUsuarios);
                break;
            case 0:
                cout << "Hasta pronto!\n";
                break;
            default:
                cout << "[!] Opcion invalida.\n";
        }
    } while (opcion != 0);

    return 0;
}