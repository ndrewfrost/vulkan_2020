/*
 *
 * Andrew Frost
 * camera.h
 * 2020
 *
 */

#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>

namespace tools {

///////////////////////////////////////////////////////////////////////////
// Camera
///////////////////////////////////////////////////////////////////////////

class Camera
{
public:
    enum Modes {
        Examine, Fly, Walk, Trackball
    };
    enum Actions {
        None, Orbit, Dolly, Pan, LookAround
    };
    struct Inputs {
        bool lmb = false; bool mmb = false; bool rmb = false;
        bool shift = false; bool ctrl = false; bool alt = false;
    };
    
    Actions mouseMove(int x, int y, const Inputs& inputs);

    void setLookAt();

    void setWindowSize();

    void setMousePosition(int x, int y);

    void getLookAt() const;

    void setMode(Modes mode);

    Modes getMode() const;

    void setRoll(float roll);

    float getRoll() const;

    const glm::mat4& getMatrix() const;

    void setSpeed(float speed);

    float getSpeed();

    void getMousePosition(int& x, int& y);

    void motion(int x, int y, int action = 0);

    void wheel(int value);

    int getWidth() const;

    int getHeight() const;

protected:
    Camera();

private:
    void update();

    void pan();

    void orbit();

    void dolly();

    void tackball();

    double projectOnToTBSphere();

protected:
    // Camera Position
    glm::vec3 m_pos    = glm::vec3(1, 1, 1);
    glm::vec3 m_int    = glm::vec3(0, 0, 0);
    glm::vec3 m_up     = glm::vec3(0, 1, 0);
    glm::mat4 m_matrix = glm::mat4(1);
    float     m_roll   = 0; // Rotation around Z axis

    // Screen 
    int m_width  = 1;
    int m_height = 1;

    // Other
    float     m_speed = 30;
    glm::vec2 m_mouse = glm::vec2(0, 0);

    bool  m_button = false;  // Button pressed
    bool  m_moving = false;  // Mouse is moving
    float m_tbsize = 0.8f;   // Trackball size

    Modes m_mode = Examine;


}; // class Camera

} // namespace tools

#define CameraView tools::Camera::Singleton()