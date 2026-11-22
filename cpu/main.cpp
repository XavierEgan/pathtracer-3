#include <string.h>
#include <glm/glm.hpp>
#include <glm/ext/scalar_relational.hpp>
#include <vector>


class Frame {
    char* data;
    int width;
    int height;

    Frame(int x, int y) {
        data = new char[x * y * 3];
    }
    ~Frame() {
        delete[] data;
    }
    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;
    Frame(Frame&&) = delete;
    Frame& operator=(Frame&&) = delete;

    void write_pixel(int x, int y, glm::u8vec3& color) {
        data[(y * width + x) * 3] = color.r;
        data[(y * width + x) * 3 + 1] = color.g;
        data[(y * width + x) * 3 + 2] = color.b;
    }
};

class SwapChain {
    Frame cur_frame;
    Frame rendering_frame;
};

class Material {
    glm::vec3 color;
    float roughness;
};

class Scene {
    std::vector<glm::vec3> verts;
    std::vector<int> indices;

    void add_tri(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2) {
        // winded v2 -> v1 -> v0
        // CCW winding
        add_vert(v0);
        add_vert(v1);
        add_vert(v2);
        
        add_index(verts.size() - 1);
        add_index(verts.size() - 2);
        add_index(verts.size() - 3);
    }

    void add_vert(const glm::vec3& v) {
        verts.push_back(v);
    }

    void add_index(int i) {
        indices.push_back(i);
    }
};

class Ray {
    glm::vec3 origin;
    glm::vec3 direction;

    bool intersect(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2) {
        
    }
};

class Camera {
    Ray center;
    int width;
    int height;
    float horizontal_fov;
    float vertical_fov;
};



void render(Frame& frame, Scene& scene) {

}

int main() {

}