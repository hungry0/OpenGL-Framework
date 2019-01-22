#pragma once

#include <glad\glad.h>

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

class Shader {
public:
	unsigned int ID;

	Shader(const GLchar* vertexPath, const GLchar* fragmentPath)
	{
        // debug info
        std::cout << vertexPath << std::endl;
        std::cout << fragmentPath << std::endl;
        // debug info end

		std::string vertexCode;
		std::string fragmentCode;

		std::ifstream vShaderFile;
		std::ifstream fShaderFile;

		vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

		try {
			vShaderFile.open(vertexPath);
			fShaderFile.open(fragmentPath);

			std::stringstream vShaderSteam, fShaderSteam;
			
			vShaderSteam << vShaderFile.rdbuf();
			fShaderSteam << fShaderFile.rdbuf();

			vShaderFile.close();
			fShaderFile.close();

			vertexCode = vShaderSteam.str();
			fragmentCode = fShaderSteam.str();

		}
		catch (std::ifstream::failure e)
		{
            std::cout << "SYSTEM IO OUT: " << e.code() << std::endl;
			std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULL_READ" << std::endl;
		}

		const char* vShaderCode = vertexCode.c_str();
		const char* fShaderCode = fragmentCode.c_str();

		unsigned int vertex, fragment;
		vertex = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertex, 1, &vShaderCode, NULL);
		glCompileShader(vertex);
		checkCompileErrors(vertex, "VERTEX");

		fragment = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment, 1, &fShaderCode, NULL);
		glCompileShader(fragment);
		checkCompileErrors(fragment, "FRAGMENT");

		ID = glCreateProgram();
		glAttachShader(ID, vertex);
		glAttachShader(ID, fragment);
		glLinkProgram(ID);
		checkCompileErrors(ID, "PROGRAM");

		glDeleteShader(vertex);
		glDeleteShader(fragment);
		
	}

	void use() {
		glUseProgram(ID);
	}

	void setBool(const std::string &name, bool value) const
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
	}
	void setInt(const std::string &name, int value) const 
	{
		glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
	}
	void setFloat(const std::string &name, float value) const 
	{
		glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
	}
	void setVector(const std::string &name, float r, float g, float b, float a) const 
	{
		glUniform4f(glGetUniformLocation(ID, name.c_str()), r, g, b, a);
	}
	void setVec3(const std::string &name, glm::vec3 & value) const
	{
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
	}
	void setMat4(const std::string &name, glm::mat4 value) const {
		glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(value));
	}

private:
	void checkCompileErrors(GLuint shader, std::string type)
	{
		GLint success;
		GLchar infoLog[1024];

		if (type == "VERTEX" || type == "FRAGMENT")
		{
			glGetProgramiv(shader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR:SHADER_COMPILE_ERROR of type:" << type << infoLog << std::endl;
			}
		}
		else if (type == "PROGRAM")
		{
			glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if (!success)
			{
				glGetProgramInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR:SHADER_LINK_ERROR of type:" << type << infoLog << std::endl;
			}
		}
		else {
			std::cout << "ERROR:PARAM_HAS_ERROR" << type << std::endl;
		}
	}
};
