//
//  Shader.hpp
//  Lab3
//
//  Created by CGIS on 05/10/2016.
//  Copyright © 2016 CGIS. All rights reserved.
//

#ifndef Shader_hpp
#define Shader_hpp

#include <stdio.h>
#include <iostream>
#include <GL/glew.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <unordered_map>

namespace gps {

// glGetUniformLocation with a cache: each (program, name) pair is looked up in GL only once.
// The scene sets most uniforms every frame, so this saves a GL string lookup per uniform per frame.
GLint uniformLocation(GLuint program, const std::string& name);

class Shader
{
public:
    GLuint shaderProgram;
    void loadShader(std::string vertexShaderFileName, std::string fragmentShaderFileName);
    void useShaderProgram();

private:
    std::string readShaderFile(std::string fileName);
    void shaderCompileLog(GLuint shaderId);
    void shaderLinkLog(GLuint shaderProgramId);
};

}

#endif /* Shader_hpp */
