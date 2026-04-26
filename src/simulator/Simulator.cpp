///   File: Simulator.cpp

#include"Simulator.h"
using namespace std;

Simulator &Simulator::getInstance()
{
    static Simulator instanceSimulator;

    return instanceSimulator;
}

void Simulator::run()
{
    objects.reserve(maxNumberOfObjects);

    cout << "Simulator is running" << endl;
    EngineCore::run();
}

void Simulator::setupProjection()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = (float)width / (float)height;

    if (!projectionOrtho)
    {
        // Perspective: near=0.5, far=5000 (scaled by 10x world)
        glFrustum(-0.1 * aspect, 0.1 * aspect, -0.1, 0.1, 0.5, 5000.0);
    }
    else
    {
        // Orthographic: zoom-based
        float w = orthoZoom * aspect;
        float h = orthoZoom;
        glOrtho(-w, w, -h, h, -5000.0, 5000.0);
    }
    glMatrixMode(GL_MODELVIEW);
}

void Simulator::updateCameraVectors()
{
    float yawRad = cameraRot.x * M_PI / 180.0f;
    float pitchRad = cameraRot.y * M_PI / 180.0f;
    float cosPitch = cosf(pitchRad);

    // Front vector from yaw/pitch
    cameraFront.x = sinf(yawRad) * cosPitch;
    cameraFront.y = -sinf(pitchRad);
    cameraFront.z = -cosf(yawRad) * cosPitch;
    cameraFront.normalize();

    // Right = front x worldUp
    Vec3 worldUp(0, 1, 0);
    cameraRight = Vec3::cross(cameraFront, worldUp);
    cameraRight.normalize();

    // Up = right x front
    cameraUp = Vec3::cross(cameraRight, cameraFront);
    cameraUp.normalize();
}

void Simulator::redraw()
{
    // Update projection each frame (handles toggle)
    setupProjection();

    // Build lookAt view matrix manually (column-major for OpenGL)
    updateCameraVectors();

    // eye = cameraPos, center = cameraPos + front, up = cameraUp
    // Build rotation part from right/up/forward basis
    Vec3 f = cameraFront;
    Vec3 r = cameraRight;
    Vec3 u = cameraUp;

    // View matrix = rotation * translation
    // OpenGL column-major: m[col*4 + row]
    float view[16];
    view[0] = r.x;   view[4] = r.y;   view[8]  = r.z;   view[12] = -(r.x*cameraPos.x + r.y*cameraPos.y + r.z*cameraPos.z);
    view[1] = u.x;   view[5] = u.y;   view[9]  = u.z;   view[13] = -(u.x*cameraPos.x + u.y*cameraPos.y + u.z*cameraPos.z);
    view[2] = -f.x;  view[6] = -f.y;  view[10] = -f.z;  view[14] =  (f.x*cameraPos.x + f.y*cameraPos.y + f.z*cameraPos.z);
    view[3] = 0;     view[7] = 0;     view[11] = 0;     view[15] = 1;

    glLoadIdentity();

    // Apply the lookAt view matrix
    glMultMatrixf(view);

    // World scale: 1 world unit = 10 GL units
    scale(10, 10, 10);

    // Z-axis flip for coordinate convention
    scale(1, 1, -1);

    // ---- Ground plane ----
    setColor(Road::groundColor);
    float gs = 60.0f;
    float gy = -(Road::ROAD_DEPTH + 0.005f);
    beginDraw(QUADS);
    setNormal(0, -1, 0);
    drawVertex(Vec3(-gs, gy, -gs));
    drawVertex(Vec3(gs, gy, -gs));
    drawVertex(Vec3(gs, gy, gs));
    drawVertex(Vec3(-gs, gy, gs));
    endDraw();

    pushMatrix();

    for (const auto &object : objects)
    {
        object->drawObject();
    }

    popMatrix();
}

Simulator::Simulator() : maxNumberOfObjects(0), CAMERA_VELOCITY(3)
{
    cout << "Initializing simulator...  ";

    init();

    cameraPos = Vec3(-5.5, 2.5, -7.84);
    cameraRot = Vec3(-215, 13.2, 0);

    cameraDirection = 0;
    moveSpeed = 10.0f;
    mouseSensitivity = 0.1f;
    projectionOrtho = false;
    orthoZoom = 30.0f;

    updateCameraVectors();

    cout << "Success" << endl;
}

void Simulator::keyHeld(char k)
{
    if (k >= 'A' && k <= 'Z')
    {
        k += 32;
        cameraVelocity = moveSpeed * 4.0f;  // Shift = 4x speed
    }
    else
    {
        cameraVelocity = moveSpeed;
    }

    switch (k)
    {
        case 'w': cameraDirection |= (1 << FORWARD);    break;
        case 's': cameraDirection |= (1 << BACK);       break;
        case 'a': cameraDirection |= (1 << LEFT);       break;
        case 'd': cameraDirection |= (1 << RIGHT);      break;
        case 'e': cameraDirection |= (1 << UP);         break;
        case 'q': cameraDirection |= (1 << DOWN);       break;
        case ' ': cameraDirection |= (1 << UP);          break;  // Space = up
    }
}

void Simulator::keyPressed(char k)
{
    if (k == 27)
    {
        cout << "Stopping simulator" << endl;
        cleanSimulation();
        breakMainLoop();
        return;
    }

    if (k >= 'A' && k <= 'Z') k += 32;

    switch (k)
    {
        case 'y': updatesPerFrame++;    break;
        case 't': updatesPerFrame--;    break;
        case 'h': timeScale += 0.1;     break;
        case 'g': timeScale -= 0.1;     break;
        case 'p':
            projectionOrtho = !projectionOrtho;
            cout << "Projection: " << (projectionOrtho ? "Orthographic" : "Perspective") << endl;
            break;
    }

    updatesPerFrame = max(MIN_UPDATES_PER_FRAME, min(updatesPerFrame, MAX_UPDATES_PER_FRAME));
    timeScale = max(MIN_TIME_SCALE, min(timeScale, MAX_TIME_SCALE));
}

void Simulator::cameraMove(const float delta)
{
    float speed = cameraVelocity * delta;

    // Vector-based FPS movement using derived camera vectors
    if ((cameraDirection >> FORWARD) & 1)
    {
        cameraPos = cameraPos + cameraFront * speed;
    }
    if ((cameraDirection >> BACK) & 1)
    {
        cameraPos = cameraPos - cameraFront * speed;
    }
    if ((cameraDirection >> LEFT) & 1)
    {
        cameraPos = cameraPos - cameraRight * speed;
    }
    if ((cameraDirection >> RIGHT) & 1)
    {
        cameraPos = cameraPos + cameraRight * speed;
    }
    if ((cameraDirection >> UP) & 1)
    {
        cameraPos.y += speed;
    }
    if ((cameraDirection >> DOWN) & 1)
    {
        cameraPos.y -= speed;
    }
    cameraDirection = 0;
}

void Simulator::mouseMove(const int dx, const int dy)
{
    cameraRot.x += dx * mouseSensitivity;
    cameraRot.y += dy * mouseSensitivity;

    // Clamp pitch to prevent gimbal lock
    if (cameraRot.y > 89.0f) cameraRot.y = 89.0f;
    else if (cameraRot.y < -89.0f) cameraRot.y = -89.0f;

    updateCameraVectors();
}


void Simulator::singleUpdate(const float delta)
{
    cameraMove(delta);
}

void Simulator::update(const float delta)
{
    for (auto &spot : spots)
    {
        if (spot->checkReadyToSpot())
        {
            registerObject(spot->spotVeh());
        }
        if (spot->checkReadyToDelete())
        {
            destroyObject(spot->deleteVeh());
        }
    }

    for (auto &object : objects)
    {
        object->updateObject(delta);
    }
}

void Simulator::registerObject(GameObject *go)
{
    objects.push_back(go);
}

void Simulator::destroyObject(GameObject *go)
{
     auto objectToRemove = find_if(objects.begin(), objects.end(), [&go] (GameObject *item) {return item == go;});
     iter_swap(objectToRemove, objects.end() - 1);
     objects.pop_back();
}

void Simulator::cleanSimulation()
{
    maxNumberOfObjects = 0;

    while (objects.size() > 0)
        destroyObject(objects.back());
}

GameObject* Simulator::findObjectByName(const string objectName) const
{
    auto foundObject = find_if(objects.begin(), objects.end(), [&objectName] (GameObject *item) {return item->id.compare(objectName) == 0;} );

    if (foundObject != objects.end()) return *foundObject;
    return nullptr;
}

void Simulator::loadedNewObject(GameObject *newGameObject)
{
    objects.push_back(newGameObject);
    maxNumberOfObjects++;
}

void Simulator::loadedNewFactory(Garage *newFactory)
{
    spots.push_back(newFactory);
    maxNumberOfObjects += newFactory->maxVehicles;
}
