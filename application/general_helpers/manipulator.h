/*
 *
 * Andrew Frost
 * manipulator.h
 * 2020
 *
 */

#pragma once

#include <array>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <chrono>

namespace tools {

///////////////////////////////////////////////////////////////////////////
// Manipulator                                                           //
///////////////////////////////////////////////////////////////////////////
//  The camera object can                                                //
//  - Orbit        (LMB)                                                 //
//  - Pan          (LMB + CTRL  | MMB)                                   //
//  - Dolly        (LMB + SHIFT | RMB)                                   //
//  - Look Around  (LMB + ALT   | LMB + CTRL + SHIFT)                    //
//  - Trackball                                                          //
//                                                                       //
//  In 3 modes                                                           //
//  - Examine, Fly, Walk                                                 //
///////////////////////////////////////////////////////////////////////////

class Manipulator
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

    void setLookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up, bool instantSet = true);

    void updateAnim();

    void fit(const glm::vec3& boxMin, const glm::vec3& boxMax, bool instantFit = true);

    void setWindowSize(int w, int h);

    void setMousePosition(int x, int y);

    static Manipulator& Singleton()
    {
        static Manipulator manipulator;
        return manipulator;
    }

    void getLookAt(glm::vec3& eye, glm::vec3& center, glm::vec3& up) const;

    void setMode(Modes mode);

    Modes getMode() const;

    void setRoll(float roll);

    float getRoll() const;

    void setMatrix(const glm::mat4& mat_, bool instantSet = true, float centerDistance = 1.f);

    const glm::mat4& getMatrix() const;

    void setSpeed(float speed);

    float getSpeed();

    void getMousePosition(int& x, int& y);

    void motion(int x, int y, int action = 0);

    void wheel(int value, const Inputs& inputs);

    int getWidth() const;

    int getHeight() const;

    void setFov(float fov);

    float getFov() const;

    double getDuration() const;

    void   setDuration(double val);

protected:
    Manipulator();

private:
    void update();

    void pan(float dx, float dy);

    void orbit(float dx, float dy, bool invert = false);

    void dolly(float dx, float dy);

    void trackball(int x, int y);

    double projectOntoTBSphere(const glm::vec2& p);
    double getSystemTime();

    glm::vec3 computeBezier(float t, glm::vec3& p0, glm::vec3& p1, glm::vec3& p2);
    void      findBezierPoints();

protected:
    struct Camera
    {
        glm::vec3 eye = glm::vec3(1, 1, 1);
        glm::vec3 ctr = glm::vec3(0, 0, 0);
        glm::vec3 up = glm::vec3(0, 1, 0);;
    };

    glm::mat4 m_matrix = glm::mat4(1);
    float     m_roll = 0; // Rotation around Z axis
    float     m_fov = 60.f;

    Camera m_current;  // current Camera Position
    Camera m_goal;     // Wish Camera Position
    Camera m_snapshot; // Current camera the moment a set lookat is done

    std::array<glm::vec3, 3> m_bezier;
    double                   m_start_time = 0;
    double                   m_duration = 0.5;

    // Screen 
    int m_width = 1;
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

#define CameraManipulator tools::Manipulator::Singleton()