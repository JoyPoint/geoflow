#include <iostream>
#include <string>
#include <fstream>
#include <cassert>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "gloo.h"

class App
{
public:
  App  (int width, int height, std::string title);
  // ~app ();

   virtual void on_initialise(){};
   void run();
   virtual void on_draw(){};
  
   virtual void on_resize(int new_width, int new_height){};  
   virtual void on_key_press(int key, int action, int mods){};
   virtual void on_mouse_wheel(double xoffset, double yoffset){};
   virtual void on_mouse_press(int button, int action, int mods){};
   virtual void on_mouse_move(double xpos, double ypos){};

  int width;
  int height;
  GLuint VBO, VAO;
  GLuint mProgram;
  GLint  mStatus;
  GLint  mLength;

protected:
  static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
  static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);
  static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
  static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
  static void window_size_callback(GLFWwindow* window, int new_width, int new_height);
  static void error_callback(int error, const char* description);
  static void char_callback(GLFWwindow*, unsigned int c);

  GLFWwindow* window;
};
