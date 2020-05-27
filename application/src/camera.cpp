/*
 *
 * Andrew Frost
 * camera.cpp
 * 2020
 *
 */

#include "camera.h"

namespace tools {

//--------------------------------------------------------------------------------------------------
// Math Functions
//
template <typename T>
bool isZero(const T& _a)
{
    return fabs(_a) < std::numeric_limits<T>::epsilon();
}

template <typename T>
bool isOne(const T& _a)
{
    return areEqual(_a, (T)1);
}

inline float sign(float s)
{
    return (s < 0.f) ? -1.f : 1.f;
}

///////////////////////////////////////////////////////////////////////////
// Camera
///////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------------
// 
//
Camera::Camera()
{
    update();
}


//--------------------------------------------------------------------------------------------------
// update internal transformation matrix
//
void Camera::update()
{
    m_matrix = glm::lookAt(m_pos, m_int, m_up);

    if (!isZero(m_roll)) {
        glm::mat4 rotate = glm::rotate(m_roll, glm::vec3(0, 0, 1));
        m_matrix = m_matrix * rotate;
    }
}

//--------------------------------------------------------------------------------------------------
// pan the camera perpendicularly to the line of sight
//
void Camera::pan(float dx, float dy)
{
    if (m_mode == Fly) {
        dx *= -1;
        dy *= -1;
    }

    glm::vec3 z(m_pos - m_int);
    float length = static_cast<float>(glm::length(z)) / 0.785f;  // 45 degrees
    z = glm::normalize(z);
    glm::vec3 x = glm::cross(m_up, z);
    x = glm::normalize(x);
    glm::vec3 y = glm::cross(z, x);
    y = glm::normalize(y);

    x *= -dx * length;
    y *= dy * length;

    m_pos += x + y;
    m_int += x + y;

}

//--------------------------------------------------------------------------------------------------
// Orbit camera around interest point
// if 'invert' then camera stays in place and interest orbit around the camera
//
void Camera::orbit(float dx, float dy, bool invert)
{
    if (isZero(dx) && isZero(dy)) {
        return;
    }

    // Full width will do a full turn
    dx *= float(glm::two_pi<float>());
    dy *= float(glm::two_pi<float>());

    // Get the camera
    glm::vec3 origin(invert ? m_pos : m_int);
    glm::vec3 position(invert ? m_int : m_pos);

    // Get the length of sight
    glm::vec3 centerToEye(position - origin);
    float radius = glm::length(centerToEye);
    centerToEye = glm::normalize(centerToEye);

    glm::mat4 rot_x, rot_y;

    // Find the rotation around the UP axis (Y)
    glm::vec3 axe_z(glm::normalize(centerToEye));
    rot_y = glm::rotate(-dx, m_up);

    // Apply the (Y) rotation to the eye-center vector
    glm::vec4 vect_tmp = rot_y * glm::vec4(centerToEye.x, centerToEye.y, centerToEye.z, 0);
    centerToEye = glm::vec3(vect_tmp.x, vect_tmp.y, vect_tmp.z);

    // Find the rotation around the X vector: cross between eye-center and up (X)
    glm::vec3 axe_x = glm::cross(m_up, axe_z);
    axe_x = glm::normalize(axe_x);
    rot_x = glm::rotate(-dy, axe_x);

    // Apply the (X) rotation to the eye-center vector
    vect_tmp = rot_x * glm::vec4(centerToEye.x, centerToEye.y, centerToEye.z, 0);
    glm::vec3 vect_rot(vect_tmp.x, vect_tmp.y, vect_tmp.z);
    if (sign(vect_rot.x) == sign(centerToEye.x)) {
        centerToEye = vect_rot;
    }

    // Make the vector as long as it was originally
    centerToEye *= radius;

    // Finding the new position
    glm::vec3 newPosition = centerToEye + origin;

    if (!invert) {
        m_pos = newPosition;  // Normal: change the position of the camera
    }
    else {
        m_int = newPosition;  // Inverted: change the interest point
    }
}

//--------------------------------------------------------------------------------------------------
// Move camera towards interest point (Doesn't cross it)
//
void Camera::dolly(float dx, float dy)
{
    glm::vec3 z(m_pos - m_int);
    float length = static_cast<float>(glm::length(z));

    if (isZero(length)) {
        // We are at point of interest
        return;
    }

    float dd;
    if (m_mode != Examine) {
        dd = -dy;
    }
    else {
        dd = fabs(dx) > fabs(dy) ? dx : -dy;
    }

    float factor = m_speed * dd / length;

    // adjust speed based on direction
    length /= 10;
    length = length < 0.001f ? 0.001f : length;
    factor *= length;

    // Don't move to or through the point of interest.
    if (factor >= 1.0f) {
        return;
    }

    z *= factor;

    // Not going up
    if (m_mode == Walk) {
        if (m_up.y > m_up.z) {
            z.y = 0;
        }
        else {
            z.z = 0;
        }
    }

    m_pos += z;

    // In fly mode, the interest moves with us.
    if (m_mode != Examine) {
        m_int += z;
    }
}

//--------------------------------------------------------------------------------------------------
// Trackball calculation
// Calc axis and angle by the given mouse coordinates
// Project the point onto the virtual trackball, then calculate the axis of rotation
// which is cross product of (p0, p1) and (center of ball, p0)
//
// NOTE: This is a deformed trackball -- is a trackball in the center, but is deformed into a
// hyperbolic sheet of rotation away from the center.
//
void Camera::trackball(int x, int y)
{
    glm::vec2 p0(2 * (m_mouse[0] - m_width / 2) / double(m_width),
        2 * (m_height / 2 - m_mouse[1]) / double(m_height));

    glm::vec2 p1(2 * (x - m_width / 2) / double(m_width),
        2 * (m_height / 2 - y) / double(m_height));

    // determine the z coordinate on the sphere
    glm::vec3 pTB0(p0[0], p0[1], projectOntoTBSphere(p0));
    glm::vec3 pTB1(p1[0], p1[1], projectOntoTBSphere(p1));

    // calculate the rotation axis via cross product between p0 and p1
    glm::vec3 axis = glm::cross(pTB0, pTB1);
    axis = glm::normalize(axis);

    // calculate the angle
    float t = glm::length(pTB0 - pTB1) / (2.f * m_tbsize);

    // clamp between -1 and 1
    if (t > 1.0f) {
        t = 1.0f;
    }
    else if (t < -1.0f) {
        t = -1.0f;
    }

    float rad = 2.0f * asin(t);

    glm::vec4 rot_axis = m_matrix * glm::vec4(axis, 0);
    glm::mat4 rot_mat = glm::rotate(rad, glm::vec3(rot_axis.x, rot_axis.y, rot_axis.z));
    glm::vec3 pnt = m_pos - m_int;

    glm::vec4 pnt2 = rot_mat * glm::vec4(pnt.x, pnt.y, pnt.z, 1);
    m_pos = m_int + glm::vec3(pnt2.x, pnt2.y, pnt2.z);
    glm::vec4 up2 = rot_mat * glm::vec4(m_up.x, m_up.y, m_up.z, 0);
    m_up = glm::vec3(up2.x, up2.y, up2.z);
}

//--------------------------------------------------------------------------------------------------
// Project x,y pair onto a sphere of radius r OR a hyperbolic sheet
// if we are away from the center of the sphere
//
double Camera::projectOntoTBSphere(const glm::vec2& p)
{
    double z;
    double d = length(p);
    // inside the sphere
    if (d < m_tbsize * 0.70710678118654752440) {
        z = sqrt(m_tbsize * m_tbsize - d * d);
    }
    // outside the sphere
    else {
        // on hyperbola
        double t = m_tbsize / 1.41421356237309504880;
        z = t * t / d;
    }

    return z;
}

//--------------------------------------------------------------------------------------------------
// Call when mouse is moving
// Finds the appropriate camera operator based on mouse button pressed
// returns the action that was activated
//
Camera::Actions Camera::mouseMove(int x, int y, const Inputs& inputs)
{
    Actions currAction = None;

    if (inputs.lmb) {
        if (((inputs.ctrl) && (inputs.shift)) || inputs.alt)
        {
            currAction = m_mode == Examine ? LookAround : Orbit;
        }
        else if (inputs.shift)
        {
            currAction = Dolly;
        }
        else if (inputs.ctrl)
        {
            currAction = Pan;
        }
        else
        {
            currAction = m_mode == Examine ? Orbit : LookAround;
        }
    }
    else if (inputs.mmb) {
        currAction = Pan;
    }
    else if (inputs.rmb) {
        currAction = Dolly;
    }

    if (currAction != None) {
        motion(x, y, currAction);
    }

    return currAction;
}

//--------------------------------------------------------------------------------------------------
// Function for when camera moves
//
void Camera::motion(int x, int y, int action)
{
    float dx = float(x - m_mouse[0]) / float(m_width);
    float dy = float(x - m_mouse[1]) / float(m_height);

    switch (action) {
    case Camera::Orbit:
        if (m_mode == Trackball) {
            orbit(dx, dy, true);
        }
        else {
            orbit(dx, dy, false);
        }
        break;
    case Camera::Dolly:
        dolly(dx, dy);
        break;
    case Camera::Pan:
        pan(dx, dy);
        break;
    case Camera::LookAround:
        if (m_mode == Trackball) {
            trackball(x, y);
        }
        else {
            orbit(dx, -dy, true);
        }
        break;
    }

    update();

    m_mouse[0] = static_cast<float>(x);
    m_mouse[1] = static_cast<float>(y);
}

//--------------------------------------------------------------------------------------------------
// Trigger a dolly when the wheel changes
//
void Camera::wheel(int value)
{
    auto fval(static_cast<float>(value));
    float dx = (fval * fabs(fval)) / static_cast<float>(m_width);

    glm::vec3 z(m_pos - m_int);
    float length = z.length() * 0.1f;
    length = length < 0.001f ? 0.001f : length;

    dolly(dx * m_speed, dx * m_speed);
    update();
}

//--------------------------------------------------------------------------------------------------
// Set camera information and derive viewing matrix
//
void Camera::setLookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up)
{
    m_pos = eye;
    m_int = center;
    m_up = up;
    update();
}

//--------------------------------------------------------------------------------------------------
// Set window size, call when the size of the window changes
//
void Camera::setWindowSize(int w, int h)
{
    m_width = w;
    m_height = h;
}

//--------------------------------------------------------------------------------------------------
// Set current mouse position
//
void Camera::setMousePosition(int x, int y)
{
    m_mouse[0] = static_cast<float>(x);
    m_mouse[1] = static_cast<float>(y);
}

//--------------------------------------------------------------------------------------------------
// Retreive current camera information
// Position, interest and up vector
//
void Camera::getLookAt(glm::vec3& eye, glm::vec3& center, glm::vec3& up) const
{
    eye = m_pos;
    center = m_int;
    up = m_up;
}

//--------------------------------------------------------------------------------------------------
// Set camera mode 
//
void Camera::setMode(Camera::Modes mode)
{
    m_mode = mode;
}

//--------------------------------------------------------------------------------------------------
// Retreive current camera mode
//
Camera::Modes Camera::getMode() const
{
    return m_mode;
}

//--------------------------------------------------------------------------------------------------
// Set the roll around z-axis
//
void Camera::setRoll(float roll)
{
    m_roll = roll;
    update();
}

//--------------------------------------------------------------------------------------------------
// Retreive the camera roll 
//
float Camera::getRoll() const
{
    return m_roll;
}

//--------------------------------------------------------------------------------------------------
// Retreive transformation matrix of camera
//
const glm::mat4& Camera::getMatrix() const
{
    return m_matrix;
}

//--------------------------------------------------------------------------------------------------
// Set the movement speed
//
void Camera::setSpeed(float speed)
{
    m_speed = speed;
}

//--------------------------------------------------------------------------------------------------
// Retreives current speed
//
float Camera::getSpeed()
{
    return m_speed;
}

//--------------------------------------------------------------------------------------------------
// Get last mouse position
//
void Camera::getMousePosition(int& x, int& y)
{
    x = static_cast<int>(m_mouse[0]);
    y = static_cast<int>(m_mouse[1]);
}

//--------------------------------------------------------------------------------------------------
// Retreives sceen width
//
int Camera::getWidth() const
{
    return m_width;
}

//--------------------------------------------------------------------------------------------------
// Retreives sceen height
//
int Camera::getHeight() const
{
    return m_height;
}


} // namespace tools