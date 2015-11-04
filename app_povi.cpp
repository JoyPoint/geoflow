#include "app_povi.h"

void poviApp::on_initialise(){

    model = glm::mat4(1.0);
    update_projection_matrix();
    update_view_matrix();
    update_radius();

    // Set up vertex data (and buffer(s)) and attribute pointers
    GLfloat vertices[] = {
        // Positions         // Colors
        0.5f, -0.5f, 0.0f,   1.0f, 0.0f, 0.0f,  // Bottom Right
       -0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,  // Bottom Left
        0.0f,  0.5f, 0.0f,   0.0f, 0.0f, 1.0f   // Top 
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    // // Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    // // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0); // Unbind VAO

    // Shader shader;
    shader.init();
    shader.attach("basic.vert");
    shader.attach("basic.frag");
    shader.link();

    // std::cout << "prog." << shader.get() << std::endl;
}

void poviApp::on_resize(int new_width, int new_height) {
    width = new_width;
    height = new_height;
    update_radius();
    update_projection_matrix();
}

void poviApp::on_draw(){
    // Render
    shader.activate();

    GLint projLoc = glGetUniformLocation(shader.get(), "u_projection"); 
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    GLint viewLoc = glGetUniformLocation(shader.get(), "u_view"); 
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    GLint modLoc = glGetUniformLocation(shader.get(), "u_model"); 
    glUniformMatrix4fv(modLoc, 1, GL_FALSE, glm::value_ptr(model));

    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void poviApp::on_mouse_press(int button, int action, int mods) {   
    
    if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT) {
        is_mouse_drag = true;
        glfwGetCursorPos(window, &drag_init_pos.x, &drag_init_pos.y);
    } else if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT) {
        is_mouse_drag = false;
        translation += translation_ondrag;
        translation_ondrag = glm::vec3(0);
    }
}


void poviApp::on_mouse_move(double xpos, double ypos) {
    if (is_mouse_drag) {
        xy_pos delta = { xpos-drag_init_pos.x, ypos-drag_init_pos.y };
        // std::cout << "in drag " << screen2view({xpos,ypos}).x << screen2view({xpos,ypos}).y << std::endl;

        double scale_ = cam_pos * std::tan(glm::radians(fov/2.));
        // multiply with inverse view matrix and apply translation in world coordinates
        glm::vec4 t_screen = glm::vec4(scale_*delta.x/radius, scale_*-delta.y/radius, 0., 0.);
        translation_ondrag =  glm::vec3( t_screen * glm::inverse(view) );
        update_view_matrix();
    }
}
void poviApp::on_mouse_wheel(double xoffset, double yoffset){
    scale *= yoffset/50 + 1;
    update_view_matrix();
}

void poviApp::update_view_matrix(){
    auto t = glm::mat4(1.0f);
    t = glm::translate(t, translation);
    t = glm::translate(t, translation_ondrag);
    t = glm::scale(t, glm::vec3(scale));
    view = glm::translate(t, glm::vec3(0.0f,0.0f,cam_pos));
}
void poviApp::update_projection_matrix(){
    projection = glm::perspective(fov, double(width)/double(height), clip_near, clip_far );       
}