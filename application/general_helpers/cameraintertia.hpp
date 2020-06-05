/*
 *
 * Andrew Frost
 * cameraintertia.hpp
 * 2020
 *
 */

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace tools {

///////////////////////////////////////////////////////////////////////////
// IntertiaCamera                                                        //
///////////////////////////////////////////////////////////////////////////

struct InertiaCamera
{
    glm::vec3 curEyePos, curFocusPos, curObjectPos;
    glm::vec3 eyePos, focusPos, objectPos;

    float tau, epsilon, eyeD, focusD, objectD;
    glm::mat4 matView;

    //-------------------------------------------------------------------------
    // 
    //
    InertiaCamera(
        const glm::vec3 eye    = glm::vec3(0.0f, 1.0f, -3.0f),
        const glm::vec3 focus  = glm::vec3(0, 0, 0),
        const glm::vec3 object = glm::vec3(0, 0, 0))
    {
        epsilon      = 0.001f;
        tau          = 0.2f;
        curEyePos    = eye;
        eyePos       = eye;
        curFocusPos  = focus;
        focusPos     = focus;
        curObjectPos = object;
        objectPos    = object;
        eyeD         = 0.0f;
        focusD       = 0.0f;
        objectD      = 0.0f;
        matView      = glm::mat4(); // identity matrix

        glm::mat4 lookAt = glm::lookAt(curEyePos, curFocusPos, glm::vec3(0,1,0));
        matView *= lookAt;
    }

    //-------------------------------------------------------------------------
    // 
    //
    void rotateH(float s, bool bPan = false)
    {
        glm::vec3 p  = eyePos;
        glm::vec3 o  = focusPos;
        glm::vec3 po = p - o;
        float l = glm::length(po);
        glm::vec3 dv = glm::cross(po, glm::vec3(0, 1, 0));
        dv *= s;
        p += dv;
        po = p - o;
        float l2 = glm::length(po);
        l = l2 - l;
        p -= (l / l2) * (po);
        eyePos = p;
        if (bPan)
            focusPos += dv;
    }

    //-------------------------------------------------------------------------
    // 
    //
    void rotateV(float s, bool bPan = false)
    {
        glm::vec3 p = eyePos;
        glm::vec3 o = focusPos;
        glm::vec3 po = p - o;
        float l = glm::length(po);
        glm::vec3 dv = glm::cross(po, glm::vec3(0, -1, 0));
        dv = glm::normalize(dv);
        glm::vec3 dv2 = glm::cross(po, dv);
        dv2 *= s;
        p += dv2;
        po = p - o;
        float l2 = glm::length(po);

        if (bPan)
            focusPos += dv2;

        // protect against gimbal lock
        if (std::fabs(glm::dot(po / l2, glm::vec3(0, 1, 0))) > 0.99) 
            return;

        l = l2 - l;
        p -= (l / l2) * (po);
        eyePos = p;
    }

    //-------------------------------------------------------------------------
    // 
    //
    void move(float s, bool bPan)
    {
        glm::vec3 p = eyePos;
        glm::vec3 o = focusPos;
        glm::vec3 po = p - o;
        po *= s;
        p -= po;
        if (bPan)
            focusPos -= po;
        eyePos = p;
    }
    //-------------------------------------------------------------------------
    // 
    //
    bool update(float dt)
    {
        if (dt > (1.0f / 60.0f))
            dt = (1.0f / 60.0f);

        bool bContinue = false;
        static glm::vec3 eyeVel = glm::vec3(0, 0, 0);
        static glm::vec3 eyeAcc = glm::vec3(0, 0, 0);
        eyeD = glm::length(curEyePos - eyePos);

        if (eyeD > epsilon) {
            bContinue = true;
            glm::vec3 dV = curEyePos - eyePos;
            eyeAcc = (-2.0f / tau) * eyeVel - dV / (tau * tau);
            // integrate
            eyeVel += eyeAcc * glm::vec3(dt, dt, dt);
            curEyePos += eyeVel * glm::vec3(dt, dt, dt);
        }
        else {
            eyeVel = glm::vec3(0, 0, 0);
            eyeAcc = glm::vec3(0, 0, 0);
        }

        static glm::vec3 focusVel = glm::vec3(0, 0, 0);
        static glm::vec3 focusAcc = glm::vec3(0, 0, 0);
        focusD = glm::length(curFocusPos - focusPos);

        if (focusD > epsilon) {
            bContinue = true;
            glm::vec3 dV = curFocusPos - focusPos;
            focusAcc = (-2.0f / tau) * focusVel - dV / (tau * tau);
            // integrate
            focusVel += focusAcc * glm::vec3(dt, dt, dt);
            curFocusPos += focusVel * glm::vec3(dt, dt, dt);
        }
        else {
            focusVel = glm::vec3(0, 0, 0);
            focusAcc = glm::vec3(0, 0, 0);
        }

        static glm::vec3 objectVel = glm::vec3(0, 0, 0);
        static glm::vec3 objectAcc = glm::vec3(0, 0, 0);
        objectD = glm::length(curObjectPos - objectPos);

        if (objectD > epsilon) {
            bContinue = true;
            glm::vec3 dV = curObjectPos - objectPos;
            objectAcc = (-2.0f / tau) * objectVel - dV / (tau * tau);
            // integrate
            objectVel += objectAcc * glm::vec3(dt, dt, dt);
            curObjectPos += objectVel * glm::vec3(dt, dt, dt);
        }
        else {
            objectVel = glm::vec3(0, 0, 0);
            objectAcc = glm::vec3(0, 0, 0);
        }

        // Camera View matrix
        glm::vec3 up(0, 1, 0);
        matView = glm::mat4();
        glm::mat4 Lookat = glm::lookAt(curEyePos, curFocusPos, up);
        matView *= Lookat;
        return bContinue;
    }
};// struct InertiaCamera
} // namespace tools