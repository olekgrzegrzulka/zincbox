#pragma once
#include <string>
#include <unordered_map>
#include <glm/gtc/type_ptr.hpp>
#include "common/types.hpp"

class Shader {
  public:
    Shader(std::string file_name);
    Shader(const char* vert, const char* frag);
    ~Shader();

    void use() const;

    void set_uniform_u32(std::string name, u32 value) const;
    void set_uniform_u32(std::string name, u32 value1, u32 value2) const;
    void set_uniform_u32(std::string name, u32 value1, u32 value2, u32 value3) const;
    void set_uniform_i32(std::string name, i32 value) const;
    void set_uniform_i32(std::string name, i32 value1, i32 value2) const;
    void set_uniform_i32(std::string name, i32 value1, i32 value2, i32 value3) const;
    void set_uniform_float(std::string name, float value) const;
    void set_uniform_float(std::string name, float value1, float value2) const;
    void set_uniform_float(std::string name, float value1, float value2, float value3) const;
    void set_uniform_mat4(std::string name, glm::mat4 value) const;

  private:
    i32 get_uniform_location(const std::string& name) const;

    std::string read_shader_file(std::string file_name) const;

    u32 compile_shader(std::string source_, i32 type) const;
    u32 compile_shader(const char* source, i32 type) const;
    u32 compile_vertex_shader(std::string source) const;
    u32 compile_fragment_shader(std::string source) const;
    u32 create_shader_program() const;

  public:
    u32 shader_program{};
    u32 vertex_shader{};
    u32 fragment_shader{};
    std::unordered_map<std::string, i32> uniforms;
};
