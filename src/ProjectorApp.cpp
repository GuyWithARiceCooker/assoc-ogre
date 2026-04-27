/**
 * assoc-projectors - kulon, asset-mentes Ogre3D projektor + fust jelenet.
 *
 * Ez szandekosan nem katamaran jelenet: kis szinpadszeru vizparti installacio,
 * ahol a projektorhazakbol lathato, attetszo fenykuppal es Ogre spotlampakkal
 * jon ki a feny, a fustpamacson keresztul.
 */

#include "Ogre.h"
#include "OgreApplicationContext.h"
#include "OgreInput.h"
#include "OgreManualObject.h"
#include "OgreMaterialManager.h"
#include "OgrePlane.h"
#include "OgreResourceGroupManager.h"
#include "OgreRTShaderSystem.h"
#include "OgreTechnique.h"

#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace
{
    void preferX11WhenWaylandOgreIsUnavailable()
    {
        const char* const forceX11 = std::getenv("ASSOC_FORCE_X11");
        const char* const sdlDriver = std::getenv("SDL_VIDEODRIVER");
        const bool explicitlyAllowWayland = forceX11 && std::strcmp(forceX11, "0") == 0;
        const bool sdlWouldUseWayland = !sdlDriver || std::strcmp(sdlDriver, "wayland") == 0;
        if (!explicitlyAllowWayland && sdlWouldUseWayland && std::getenv("WAYLAND_DISPLAY") && std::getenv("DISPLAY"))
        {
            setenv("SDL_VIDEODRIVER", "x11", 1);
        }
    }

    Ogre::MaterialPtr makeMaterial(const Ogre::String& name, const Ogre::ColourValue& diffuse,
        const Ogre::ColourValue& selfIllumination = Ogre::ColourValue::Black, const bool transparent = false)
    {
        Ogre::MaterialPtr mat = Ogre::MaterialManager::getSingleton().create(
            name, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        Ogre::Pass* pass = mat->getTechnique(0)->getPass(0);
        pass->setDiffuse(diffuse);
        pass->setAmbient(diffuse * 0.5f);
        pass->setSpecular(0.45f, 0.5f, 0.56f, 1.0f);
        pass->setShininess(28.0f);
        pass->setSelfIllumination(selfIllumination);
        if (transparent)
        {
            pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
            pass->setDepthWriteEnabled(false);
        }
        return mat;
    }

    void pushQuad(Ogre::ManualObject* mo, const Ogre::Vector3& a, const Ogre::Vector3& b,
        const Ogre::Vector3& c, const Ogre::Vector3& d, const Ogre::Vector3& normal)
    {
        mo->position(a); mo->normal(normal);
        mo->position(b); mo->normal(normal);
        mo->position(c); mo->normal(normal);
        mo->position(a); mo->normal(normal);
        mo->position(c); mo->normal(normal);
        mo->position(d); mo->normal(normal);
    }

    Ogre::ManualObject* createBox(Ogre::SceneManager* scene, const Ogre::String& name,
        const Ogre::Vector3& size, const Ogre::String& material)
    {
        const Ogre::Real hx = size.x * 0.5f;
        const Ogre::Real hy = size.y * 0.5f;
        const Ogre::Real hz = size.z * 0.5f;
        Ogre::ManualObject* mo = scene->createManualObject(name);
        mo->begin(material, Ogre::RenderOperation::OT_TRIANGLE_LIST);
        pushQuad(mo, {-hx, -hy, hz}, {hx, -hy, hz}, {hx, hy, hz}, {-hx, hy, hz}, Ogre::Vector3::UNIT_Z);
        pushQuad(mo, {hx, -hy, -hz}, {-hx, -hy, -hz}, {-hx, hy, -hz}, {hx, hy, -hz}, Ogre::Vector3::NEGATIVE_UNIT_Z);
        pushQuad(mo, {-hx, hy, hz}, {hx, hy, hz}, {hx, hy, -hz}, {-hx, hy, -hz}, Ogre::Vector3::UNIT_Y);
        pushQuad(mo, {-hx, -hy, -hz}, {hx, -hy, -hz}, {hx, -hy, hz}, {-hx, -hy, hz}, Ogre::Vector3::NEGATIVE_UNIT_Y);
        pushQuad(mo, {hx, -hy, hz}, {hx, -hy, -hz}, {hx, hy, -hz}, {hx, hy, hz}, Ogre::Vector3::UNIT_X);
        pushQuad(mo, {-hx, -hy, -hz}, {-hx, -hy, hz}, {-hx, hy, hz}, {-hx, hy, -hz}, Ogre::Vector3::NEGATIVE_UNIT_X);
        mo->end();
        return mo;
    }

    Ogre::ManualObject* createBeam(Ogre::SceneManager* scene, const Ogre::String& name,
        const Ogre::String& material, const Ogre::Real length, const Ogre::Real nearHalf,
        const Ogre::Real farHalf)
    {
        Ogre::ManualObject* mo = scene->createManualObject(name);
        mo->begin(material, Ogre::RenderOperation::OT_TRIANGLE_LIST);
        const Ogre::Vector3 n0{-nearHalf, nearHalf, 0.0f};
        const Ogre::Vector3 n1{nearHalf, nearHalf, 0.0f};
        const Ogre::Vector3 n2{nearHalf, -nearHalf, 0.0f};
        const Ogre::Vector3 n3{-nearHalf, -nearHalf, 0.0f};
        const Ogre::Vector3 f0{-farHalf, farHalf, -length};
        const Ogre::Vector3 f1{farHalf, farHalf, -length};
        const Ogre::Vector3 f2{farHalf, -farHalf, -length};
        const Ogre::Vector3 f3{-farHalf, -farHalf, -length};
        pushQuad(mo, n0, n1, f1, f0, Ogre::Vector3::UNIT_Y);
        pushQuad(mo, n1, n2, f2, f1, Ogre::Vector3::UNIT_X);
        pushQuad(mo, n2, n3, f3, f2, Ogre::Vector3::NEGATIVE_UNIT_Y);
        pushQuad(mo, n3, n0, f0, f3, Ogre::Vector3::NEGATIVE_UNIT_X);
        pushQuad(mo, f0, f1, f2, f3, Ogre::Vector3::NEGATIVE_UNIT_Z);
        mo->end();
        return mo;
    }
}

class ProjectorApp
    : public OgreBites::ApplicationContext
    , public OgreBites::InputListener
{
public:
    ProjectorApp()
        : OgreBites::ApplicationContext("assoc-projectors")
    {
    }

    bool oneTimeConfig() override
    {
        Ogre::Root* const root = getRoot();
        const Ogre::RenderSystemList& renderers = root->getAvailableRenderers();
        if (renderers.empty())
        {
            Ogre::LogManager::getSingleton().logError("No RenderSystems available");
            return false;
        }
        Ogre::RenderSystem* rs = root->getRenderSystemByName("OpenGL 3+ Rendering Subsystem");
        root->setRenderSystem(rs ? rs : renderers.front());
        setRenderOption(root->getRenderSystem(), "Full Screen", "No");
        setRenderOption(root->getRenderSystem(), "Video Mode", std::getenv("ASSOC_VIDEO_MODE") ? std::getenv("ASSOC_VIDEO_MODE") : "1024 x 640");
        setRenderOption(root->getRenderSystem(), "FSAA", "0");
        setRenderOption(root->getRenderSystem(), "VSync", "Yes");
        setRenderOption(root->getRenderSystem(), "sRGB Gamma Conversion", "No");
        setRenderOption(root->getRenderSystem(), "RTT Preferred Mode", "FBO");
        setRenderOption(root->getRenderSystem(), "Reversed Z", "No");
        return true;
    }

    void setup() override
    {
        OgreBites::ApplicationContext::setup();
        addInputListener(this);
        mScene = getRoot()->createSceneManager();
        Ogre::RTShader::ShaderGenerator::getSingleton().addSceneManager(mScene);

        createMaterials();
        mScene->setAmbientLight(Ogre::ColourValue(0.08f, 0.09f, 0.12f));
        mScene->setFog(Ogre::FOG_LINEAR, Ogre::ColourValue(0.06f, 0.07f, 0.1f), 0.001f, 95.0f, 420.0f);

        Ogre::Light* dusk = mScene->createLight("dusk");
        dusk->setType(Ogre::Light::LT_DIRECTIONAL);
        dusk->setDiffuseColour(0.85f, 0.48f, 0.28f);
        mDuskNode = mScene->getRootSceneNode()->createChildSceneNode("duskNode");
        mDuskNode->setDirection(Ogre::Vector3(-0.42f, -0.58f, -0.38f).normalisedCopy());
        mDuskNode->attachObject(dusk);

        mCamNode = mScene->getRootSceneNode()->createChildSceneNode("cam", Ogre::Vector3(0.0f, 36.0f, 164.0f));
        mCam = mScene->createCamera("main");
        mCam->setNearClipDistance(0.1f);
        mCam->setFarClipDistance(800.0f);
        mCam->setAutoAspectRatio(true);
        mCamNode->attachObject(mCam);
        Ogre::Viewport* const vp = getRenderWindow()->addViewport(mCam);
        vp->setBackgroundColour(Ogre::ColourValue(0.055f, 0.06f, 0.09f));

        buildStage();
        buildProjectors();
    }

    void shutdown() override
    {
        if (mScene)
        {
            Ogre::RTShader::ShaderGenerator::getSingleton().removeSceneManager(mScene);
        }
        mScene = nullptr;
        OgreBites::ApplicationContext::shutdown();
    }

    bool frameRenderingQueued(const Ogre::FrameEvent& evt) override
    {
        mT += evt.timeSinceLastFrame;
        for (std::size_t i = 0; i < mSmokeNodes.size(); ++i)
        {
            const Ogre::Real phase = mT * (0.34f + 0.04f * static_cast<Ogre::Real>(i)) + static_cast<Ogre::Real>(i);
            Ogre::SceneNode* const node = mSmokeNodes[i];
            node->setPosition(mSmokeBase[i] + Ogre::Vector3(Ogre::Math::Sin(phase) * 4.5f,
                Ogre::Math::Sin(phase * 0.7f) * 1.4f, Ogre::Math::Cos(phase * 0.8f) * 5.0f));
            node->yaw(Ogre::Degree(Ogre::Math::Sin(phase) * 0.55f));
            node->setScale(1.0f + 0.11f * Ogre::Math::Sin(phase), 1.0f + 0.08f * Ogre::Math::Cos(phase),
                1.0f + 0.16f * Ogre::Math::Sin(phase * 0.8f));
        }
        for (std::size_t i = 0; i < mBeamNodes.size(); ++i)
        {
            const Ogre::Real shimmer = 1.0f + 0.045f * Ogre::Math::Sin(mT * 1.7f + static_cast<Ogre::Real>(i));
            mBeamNodes[i]->setScale(shimmer, shimmer, 1.0f);
        }
        const Ogre::Real c = 14.0f * Ogre::Math::Cos(mT * 0.10f);
        mCamNode->setPosition(c, 37.0f + 2.0f * Ogre::Math::Sin(mT * 0.13f), 164.0f);
        mCamNode->lookAt(Ogre::Vector3(0.0f, 18.0f, -14.0f), Ogre::Node::TS_WORLD, Ogre::Vector3::NEGATIVE_UNIT_Z);
        return OgreBites::ApplicationContext::frameRenderingQueued(evt);
    }

    bool keyPressed(const OgreBites::KeyboardEvent& key) override
    {
        if (key.keysym.sym == OgreBites::SDLK_ESCAPE)
        {
            Ogre::Root::getSingleton().queueEndRendering();
        }
        return true;
    }

private:
    void setRenderOption(Ogre::RenderSystem* const rs, const Ogre::String& name, const Ogre::String& value)
    {
        const Ogre::ConfigOptionMap& options = rs->getConfigOptions();
        if (options.find(name) != options.end())
        {
            rs->setConfigOption(name, value);
        }
    }

    void createMaterials()
    {
        makeMaterial("projectors/stage", Ogre::ColourValue(0.08f, 0.09f, 0.1f, 1.0f));
        makeMaterial("projectors/body", Ogre::ColourValue(0.025f, 0.028f, 0.034f, 1.0f));
        makeMaterial("projectors/lens", Ogre::ColourValue(0.7f, 0.88f, 1.0f, 0.78f), Ogre::ColourValue(0.16f, 0.32f, 0.5f), true);
        makeMaterial("projectors/beamWarm", Ogre::ColourValue(1.0f, 0.70f, 0.32f, 0.20f), Ogre::ColourValue(0.24f, 0.13f, 0.03f), true);
        makeMaterial("projectors/beamCool", Ogre::ColourValue(0.46f, 0.78f, 1.0f, 0.18f), Ogre::ColourValue(0.05f, 0.11f, 0.22f), true);
        makeMaterial("projectors/smoke", Ogre::ColourValue(0.78f, 0.82f, 0.87f, 0.20f), Ogre::ColourValue(0.035f, 0.04f, 0.05f), true);
        makeMaterial("projectors/water", Ogre::ColourValue(0.14f, 0.28f, 0.42f, 0.66f), Ogre::ColourValue(0.01f, 0.04f, 0.06f), true);
    }

    void buildStage()
    {
        Ogre::MeshManager::getSingleton().createPlane("projectorWater",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::Plane(Ogre::Vector3::UNIT_Y, 0.0f),
            1200.0f, 1200.0f, 20, 20, true, 1, 10.0f, 10.0f, Ogre::Vector3::UNIT_Z);
        Ogre::Entity* water = mScene->createEntity("projectorWater");
        water->setMaterialName("projectors/water");
        mScene->getRootSceneNode()->createChildSceneNode("water")->attachObject(water);

        Ogre::SceneNode* deck = mScene->getRootSceneNode()->createChildSceneNode("stageDeck", Ogre::Vector3(0.0f, 2.0f, 0.0f));
        deck->attachObject(createBox(mScene, "stageDeckBox", Ogre::Vector3(180.0f, 3.0f, 86.0f), "projectors/stage"));
    }

    void buildProjectors()
    {
        const Ogre::Vector3 target{0.0f, 16.0f, -24.0f};
        const std::array<Ogre::Vector3, 4> projectorPos{{
            {-105.0f, 18.0f, 68.0f}, {105.0f, 18.0f, 68.0f},
            {-42.0f, 28.0f, 108.0f}, {42.0f, 28.0f, 108.0f},
        }};
        const std::array<Ogre::String, 4> beamMats{{"projectors/beamWarm", "projectors/beamCool", "projectors/beamCool", "projectors/beamWarm"}};
        const std::array<Ogre::ColourValue, 4> lightColours{{
            Ogre::ColourValue(1.0f, 0.70f, 0.35f, 1.0f), Ogre::ColourValue(0.48f, 0.78f, 1.0f, 1.0f),
            Ogre::ColourValue(0.62f, 0.86f, 1.0f, 1.0f), Ogre::ColourValue(1.0f, 0.82f, 0.42f, 1.0f),
        }};

        for (std::size_t i = 0; i < projectorPos.size(); ++i)
        {
            Ogre::SceneNode* stand = mScene->getRootSceneNode()->createChildSceneNode(
                "projectorStand" + Ogre::StringConverter::toString(i), projectorPos[i] + Ogre::Vector3(0.0f, -8.5f, 0.0f));
            stand->attachObject(createBox(mScene, "projectorStandBox" + Ogre::StringConverter::toString(i),
                Ogre::Vector3(2.0f, 17.0f, 2.0f), "projectors/body"));

            Ogre::SceneNode* projector = mScene->getRootSceneNode()->createChildSceneNode(
                "projector" + Ogre::StringConverter::toString(i), projectorPos[i]);
            projector->lookAt(target, Ogre::Node::TS_WORLD, Ogre::Vector3::NEGATIVE_UNIT_Z);
            projector->attachObject(createBox(mScene, "projectorBox" + Ogre::StringConverter::toString(i),
                Ogre::Vector3(13.0f, 7.0f, 10.0f), "projectors/body"));

            projector->createChildSceneNode("lens" + Ogre::StringConverter::toString(i),
                Ogre::Vector3(0.0f, 0.0f, -5.7f))->attachObject(createBox(mScene,
                "lensBox" + Ogre::StringConverter::toString(i), Ogre::Vector3(7.4f, 4.5f, 0.7f), "projectors/lens"));

            Ogre::SceneNode* beam = projector->createChildSceneNode("beam" + Ogre::StringConverter::toString(i),
                Ogre::Vector3(0.0f, 0.0f, -6.0f));
            beam->attachObject(createBeam(mScene, "beamMesh" + Ogre::StringConverter::toString(i),
                beamMats[i], 120.0f, 2.3f, 25.0f));
            mBeamNodes.push_back(beam);

            Ogre::Light* spot = mScene->createLight("spot" + Ogre::StringConverter::toString(i));
            spot->setType(Ogre::Light::LT_SPOTLIGHT);
            spot->setDiffuseColour(lightColours[i]);
            spot->setSpecularColour(lightColours[i]);
            spot->setSpotlightRange(Ogre::Degree(9.0f), Ogre::Degree(25.0f), 0.75f);
            spot->setAttenuation(190.0f, 1.0f, 0.004f, 0.0f);
            projector->createChildSceneNode("spotNode" + Ogre::StringConverter::toString(i),
                Ogre::Vector3(0.0f, 0.0f, -5.8f))->attachObject(spot);
        }

        Ogre::SceneNode* smokeMachine = mScene->getRootSceneNode()->createChildSceneNode("smokeMachine", Ogre::Vector3(-48.0f, 7.0f, 30.0f));
        smokeMachine->yaw(Ogre::Degree(-18.0f));
        smokeMachine->attachObject(createBox(mScene, "smokeMachineBox", Ogre::Vector3(22.0f, 8.0f, 13.0f), "projectors/body"));
        smokeMachine->createChildSceneNode("smokeNozzle", Ogre::Vector3(11.5f, 1.0f, 0.0f))->attachObject(
            createBox(mScene, "smokeNozzleBox", Ogre::Vector3(4.0f, 2.4f, 5.2f), "projectors/lens"));

        for (int i = 0; i < 18; ++i)
        {
            const Ogre::Vector3 base{-30.0f + static_cast<Ogre::Real>(i % 6) * 13.0f,
                10.0f + static_cast<Ogre::Real>(i % 5) * 4.0f,
                18.0f - static_cast<Ogre::Real>(i) * 6.0f};
            Ogre::SceneNode* smoke = mScene->getRootSceneNode()->createChildSceneNode("smokePuff" + Ogre::StringConverter::toString(i), base);
            smoke->yaw(Ogre::Degree(static_cast<Ogre::Real>((i * 23) % 90)));
            smoke->roll(Ogre::Degree(static_cast<Ogre::Real>((i * 17) % 35)));
            smoke->attachObject(createBox(mScene, "smokePuffBox" + Ogre::StringConverter::toString(i),
                Ogre::Vector3(24.0f + static_cast<Ogre::Real>(i % 3) * 8.0f, 5.4f, 11.0f), "projectors/smoke"));
            mSmokeNodes.push_back(smoke);
            mSmokeBase.push_back(base);
        }
    }

    Ogre::SceneManager* mScene{nullptr};
    Ogre::Camera* mCam{nullptr};
    Ogre::SceneNode* mCamNode{nullptr};
    Ogre::SceneNode* mDuskNode{nullptr};
    std::vector<Ogre::SceneNode*> mBeamNodes;
    std::vector<Ogre::SceneNode*> mSmokeNodes;
    std::vector<Ogre::Vector3> mSmokeBase;
    Ogre::Real mT{0.0f};
};

int main()
{
    try
    {
        preferX11WhenWaylandOgreIsUnavailable();
        ProjectorApp app;
        app.initApp();
        app.getRoot()->startRendering();
        app.closeApp();
    }
    catch (const Ogre::Exception& e)
    {
        Ogre::LogManager::getSingleton().logMessage(e.getFullDescription(), Ogre::LML_CRITICAL);
        return 1;
    }
    return 0;
}
