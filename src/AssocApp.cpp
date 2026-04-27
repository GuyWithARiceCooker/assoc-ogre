/**
 * assoc — Ogre3D 14 + Bites: konnyu, asset-mentes modularis napelemes katamaran.
 *
 * Cel: fusson Linuxon/Archon es regi, 4 GB RAM-os MacBookon is. Ezert minden
 * lathato elem ManualObject vagy egyszeru Ogre plane: nincs kulso mesh, nincs
 * textura, nincs draga post-process. A hajo vizen uszik, lassan halad/billeg,
 * Space billentyuvel a harom test "szetdokkolhato".
 */

#include "Ogre.h"
#include "OgreApplicationContext.h"
#include "OgreInput.h"
#include "OgrePlane.h"
#include "OgreRTShaderSystem.h"
#include "OgreTechnique.h"
#include "OgreManualObject.h"
#include "OgreMaterialManager.h"
#include "OgreResourceGroupManager.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <vector>

namespace
{
    constexpr Ogre::Real kWaterY = 0.0f;

    void preferX11WhenWaylandOgreIsUnavailable()
    {
        // Ogre built without OGRE_USE_WAYLAND cannot accept SDL-created Wayland
        // native windows. Prefer XWayland when GNOME exposes both displays.
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
        pass->setAmbient(diffuse * 0.55f);
        pass->setSpecular(0.55f, 0.62f, 0.7f, 1.0f);
        pass->setShininess(38.0f);
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
        mo->position(a);
        mo->normal(normal);
        mo->textureCoord(0.0f, 0.0f);
        mo->position(b);
        mo->normal(normal);
        mo->textureCoord(1.0f, 0.0f);
        mo->position(c);
        mo->normal(normal);
        mo->textureCoord(1.0f, 1.0f);

        mo->position(a);
        mo->normal(normal);
        mo->textureCoord(0.0f, 0.0f);
        mo->position(c);
        mo->normal(normal);
        mo->textureCoord(1.0f, 1.0f);
        mo->position(d);
        mo->normal(normal);
        mo->textureCoord(0.0f, 1.0f);
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

    Ogre::ManualObject* createTaperedHull(Ogre::SceneManager* scene, const Ogre::String& name,
        const Ogre::String& material)
    {
        const Ogre::Real length = 74.0f;
        const Ogre::Real width = 8.0f;
        const Ogre::Real height = 7.0f;
        const Ogre::Real bow = length * 0.5f;
        const Ogre::Real stern = -length * 0.5f;
        const Ogre::Real mid = 0.0f;
        const Ogre::Real top = height * 0.5f;
        const Ogre::Real keel = -height * 0.5f;

        std::array<Ogre::Vector3, 8> v{{
            {stern, top, width * 0.45f}, {mid, top + 1.1f, width * 0.5f}, {bow, top, 0.0f},
            {stern, top, -width * 0.45f}, {mid, top + 1.1f, -width * 0.5f}, {bow, top, 0.0f},
            {stern + 4.0f, keel, 0.0f}, {mid + 10.0f, keel - 1.0f, 0.0f}
        }};

        Ogre::ManualObject* mo = scene->createManualObject(name);
        mo->begin(material, Ogre::RenderOperation::OT_TRIANGLE_LIST);
        pushQuad(mo, v[0], v[1], v[4], v[3], Ogre::Vector3::UNIT_Y);
        pushQuad(mo, v[0], v[6], v[7], v[1], Ogre::Vector3(0.0f, 0.35f, 0.94f).normalisedCopy());
        pushQuad(mo, v[4], v[7], v[6], v[3], Ogre::Vector3(0.0f, 0.35f, -0.94f).normalisedCopy());
        pushQuad(mo, v[1], v[7], v[2], v[2], Ogre::Vector3(0.8f, 0.2f, 0.0f).normalisedCopy());
        pushQuad(mo, v[4], v[5], v[7], v[7], Ogre::Vector3(0.8f, 0.2f, 0.0f).normalisedCopy());
        pushQuad(mo, v[0], v[3], v[6], v[6], Ogre::Vector3::NEGATIVE_UNIT_X);
        mo->end();
        return mo;
    }

    Ogre::ManualObject* createDisc(Ogre::SceneManager* scene, const Ogre::String& name, const Ogre::Real radius,
        const Ogre::String& material, const unsigned segments = 48U)
    {
        Ogre::ManualObject* mo = scene->createManualObject(name);
        mo->begin(material, Ogre::RenderOperation::OT_TRIANGLE_LIST);
        for (unsigned i = 0; i < segments; ++i)
        {
            const Ogre::Real a0 = Ogre::Math::TWO_PI * static_cast<Ogre::Real>(i) / static_cast<Ogre::Real>(segments);
            const Ogre::Real a1 = Ogre::Math::TWO_PI * static_cast<Ogre::Real>(i + 1U) / static_cast<Ogre::Real>(segments);
            mo->position(0.0f, 0.0f, 0.0f);
            mo->normal(Ogre::Vector3::UNIT_Z);
            mo->position(radius * Ogre::Math::Cos(a0), radius * Ogre::Math::Sin(a0), 0.0f);
            mo->normal(Ogre::Vector3::UNIT_Z);
            mo->position(radius * Ogre::Math::Cos(a1), radius * Ogre::Math::Sin(a1), 0.0f);
            mo->normal(Ogre::Vector3::UNIT_Z);
        }
        mo->end();
        return mo;
    }
} // namespace

class AssocApp
    : public OgreBites::ApplicationContext
    , public OgreBites::InputListener
{
public:
    AssocApp()
        : OgreBites::ApplicationContext("assoc")
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
        if (!rs)
        {
            rs = renderers.front();
        }
        root->setRenderSystem(rs);

        setRenderOption(rs, "Full Screen", "No");
        const char* const videoMode = std::getenv("ASSOC_VIDEO_MODE");
        setRenderOption(rs, "Video Mode", videoMode ? videoMode : "1024 x 640");
        setRenderOption(rs, "FSAA", "0");
        setRenderOption(rs, "VSync", "Yes");
        setRenderOption(rs, "sRGB Gamma Conversion", "No");
        setRenderOption(rs, "RTT Preferred Mode", "FBO");
        setRenderOption(rs, "Reversed Z", "No");
        return true;
    }

    void setup() override
    {
        OgreBites::ApplicationContext::setup();
        addInputListener(this);

        Ogre::Root* const root{getRoot()};
        mScene = root->createSceneManager();
        Ogre::RTShader::ShaderGenerator::getSingleton().addSceneManager(mScene);

        createMaterials();

        mScene->setAmbientLight(Ogre::ColourValue(0.18f, 0.18f, 0.22f));

        Ogre::Light* key = mScene->createLight("key");
        key->setType(Ogre::Light::LT_DIRECTIONAL);
        Ogre::SceneNode* kNode = mScene->getRootSceneNode()->createChildSceneNode("keyNode");
        kNode->setDirection(Ogre::Vector3(-0.38f, -0.62f, -0.44f).normalisedCopy());
        kNode->attachObject(key);
        key->setDiffuseColour(1.0f, 0.78f, 0.52f);

        Ogre::Light* fill = mScene->createLight("fill");
        fill->setType(Ogre::Light::LT_POINT);
        Ogre::SceneNode* fNode = mScene->getRootSceneNode()->createChildSceneNode("fillNode",
            Ogre::Vector3(-120.0f, 58.0f, 90.0f));
        fNode->attachObject(fill);
        fill->setDiffuseColour(0.22f, 0.35f, 0.55f);
        fill->setAttenuation(320.0f, 1.0f, 0.006f, 0.0f);

        mCamNode = mScene->getRootSceneNode()->createChildSceneNode("cam");
        mCamNode->setPosition(0.0f, 42.0f, 154.0f);
        mCam = mScene->createCamera("main");
        mCam->setNearClipDistance(0.1f);
        mCam->setFarClipDistance(900.0f);
        mCam->setAutoAspectRatio(true);
        mCamNode->attachObject(mCam);
        mRenderWindow = getRenderWindow();
        mScene->setFog(
            Ogre::FOG_LINEAR, Ogre::ColourValue(0.78f, 0.67f, 0.76f), 0.001f, 180.0f, 560.0f);
        {
            Ogre::Viewport* const vp = mRenderWindow->addViewport(mCam);
            vp->setBackgroundColour(Ogre::ColourValue(0.86f, 0.68f, 0.74f));
            vp->setClearEveryFrame(true);
        }

        buildWorld();
        buildCatamaran();
    }

    void shutdown() override
    {
        if (mScene)
        {
            Ogre::RTShader::ShaderGenerator::getSingleton().removeSceneManager(mScene);
        }
        mScene = nullptr;
        mRenderWindow = nullptr;
        mCam = nullptr;
        mCamNode = nullptr;
        OgreBites::ApplicationContext::shutdown();
    }

    bool frameRenderingQueued(const Ogre::FrameEvent& evt) override
    {
        mT += evt.timeSinceLastFrame;
        const Ogre::Real wave = Ogre::Math::Sin(mT * 1.4f);
        const Ogre::Real slow = Ogre::Math::Sin(mT * 0.55f);

        if (mBoatRoot)
        {
            mBoatRoot->setPosition(0.0f, 6.2f + wave * 0.75f, -12.0f + Ogre::Math::Sin(mT * 0.22f) * 8.0f);
            mBoatRoot->resetOrientation();
            mBoatRoot->yaw(Ogre::Degree(slow * 2.2f));
            mBoatRoot->roll(Ogre::Degree(wave * 1.4f));
            mBoatRoot->pitch(Ogre::Degree(Ogre::Math::Sin(mT * 0.9f) * 1.1f));
        }

        mSeparation = std::min(1.0f, std::max(0.0f,
            mSeparation + (mSeparated ? 0.7f : -0.9f) * evt.timeSinceLastFrame));
        for (std::size_t i = 0; i < mHullNodes.size(); ++i)
        {
            const Ogre::Real targetX = mHullBaseX[i] * (1.0f + mSeparation * 0.42f);
            const Ogre::Real targetZ = (i == 1U) ? 0.0f : Ogre::Math::Sin(mT * 1.2f + static_cast<Ogre::Real>(i)) * 0.75f * mSeparation;
            mHullNodes[i]->setPosition(targetX, 0.0f, targetZ);
        }

        if (mWakeNode)
        {
            mWakeNode->setScale(1.0f + 0.04f * wave, 1.0f, 1.0f + 0.08f * Ogre::Math::Abs(slow));
        }

        if (mCamNode)
        {
            const Ogre::Real c = 10.0f * Ogre::Math::Cos(mT * 0.12f);
            const Ogre::Real s = 7.0f * Ogre::Math::Sin(mT * 0.1f);
            mCamNode->setPosition(c, 40.0f + s * 0.35f, 154.0f);
            mCamNode->lookAt(Ogre::Vector3(0.0f, 12.0f, -10.0f), Ogre::Node::TS_WORLD, Ogre::Vector3::NEGATIVE_UNIT_Z);
        }
        return OgreBites::ApplicationContext::frameRenderingQueued(evt);
    }

    bool keyPressed(const OgreBites::KeyboardEvent& key) override
    {
        if (key.keysym.sym == OgreBites::SDLK_ESCAPE)
        {
            Ogre::Root::getSingleton().queueEndRendering();
        }
        if (key.keysym.sym == OgreBites::SDLK_SPACE)
        {
            mSeparated = !mSeparated;
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
        makeMaterial("assoc/water", Ogre::ColourValue(0.18f, 0.43f, 0.62f, 0.72f),
            Ogre::ColourValue(0.02f, 0.08f, 0.12f), true);
        makeMaterial("assoc/hullWhite", Ogre::ColourValue(0.86f, 0.9f, 0.92f, 1.0f));
        makeMaterial("assoc/carbon", Ogre::ColourValue(0.05f, 0.06f, 0.07f, 1.0f));
        makeMaterial("assoc/solarBlue", Ogre::ColourValue(0.42f, 0.83f, 1.0f, 0.7f),
            Ogre::ColourValue(0.04f, 0.22f, 0.3f), true);
        makeMaterial("assoc/solarLavender", Ogre::ColourValue(0.78f, 0.55f, 1.0f, 0.68f),
            Ogre::ColourValue(0.16f, 0.08f, 0.22f), true);
        makeMaterial("assoc/solarMint", Ogre::ColourValue(0.52f, 1.0f, 0.78f, 0.68f),
            Ogre::ColourValue(0.05f, 0.2f, 0.13f), true);
        makeMaterial("assoc/battery", Ogre::ColourValue(0.12f, 0.18f, 0.2f, 1.0f),
            Ogre::ColourValue(0.03f, 0.12f, 0.1f));
        makeMaterial("assoc/dockGlow", Ogre::ColourValue(1.0f, 0.82f, 0.36f, 0.86f),
            Ogre::ColourValue(0.28f, 0.16f, 0.02f), true);
        makeMaterial("assoc/wake", Ogre::ColourValue(0.86f, 0.98f, 1.0f, 0.38f),
            Ogre::ColourValue(0.08f, 0.14f, 0.16f), true);
        makeMaterial("assoc/sun", Ogre::ColourValue(1.0f, 0.62f, 0.34f, 0.85f),
            Ogre::ColourValue(0.55f, 0.23f, 0.08f), true);
        makeMaterial("assoc/skyPeach", Ogre::ColourValue(1.0f, 0.55f, 0.5f, 0.42f),
            Ogre::ColourValue(0.12f, 0.05f, 0.06f), true);
        makeMaterial("assoc/skyLilac", Ogre::ColourValue(0.64f, 0.55f, 1.0f, 0.35f),
            Ogre::ColourValue(0.06f, 0.05f, 0.12f), true);
    }

    void buildWorld()
    {
        Ogre::MeshManager::getSingleton().createPlane(
            "assocWater", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::Plane(Ogre::Vector3::UNIT_Y, kWaterY), 1600.0f, 1600.0f, 40, 40, true, 1, 20.0f, 20.0f,
            Ogre::Vector3::UNIT_Z);
        Ogre::Entity* water = mScene->createEntity("assocWater");
        water->setMaterialName("assoc/water");
        mWaterNode = mScene->getRootSceneNode()->createChildSceneNode("water");
        mWaterNode->attachObject(water);

        Ogre::ManualObject* sun = createDisc(mScene, "sunsetDisc", 26.0f, "assoc/sun");
        Ogre::SceneNode* sunNode = mScene->getRootSceneNode()->createChildSceneNode("sun", Ogre::Vector3(95.0f, 64.0f, -260.0f));
        sunNode->attachObject(sun);

        for (int i = 0; i < 11; ++i)
        {
            const Ogre::String mat = (i % 2 == 0) ? "assoc/skyPeach" : "assoc/skyLilac";
            Ogre::ManualObject* stripe = createBox(mScene, "skyStripe" + Ogre::StringConverter::toString(i),
                Ogre::Vector3(380.0f, 2.2f + static_cast<Ogre::Real>(i % 3), 0.1f), mat);
            Ogre::SceneNode* stripeNode = mScene->getRootSceneNode()->createChildSceneNode(
                "skyStripeNode" + Ogre::StringConverter::toString(i),
                Ogre::Vector3(15.0f, 82.0f - static_cast<Ogre::Real>(i) * 7.0f, -285.0f));
            stripeNode->roll(Ogre::Degree(-4.0f));
            stripeNode->attachObject(stripe);
        }
    }

    void buildCatamaran()
    {
        mBoatRoot = mScene->getRootSceneNode()->createChildSceneNode("solarCatamaran", Ogre::Vector3(0.0f, 6.0f, -12.0f));
        mHullBaseX = {{-28.0f, 0.0f, 28.0f}};
        const std::array<Ogre::String, 3> solarMats{{"assoc/solarBlue", "assoc/solarLavender", "assoc/solarMint"}};

        for (std::size_t i = 0; i < mHullBaseX.size(); ++i)
        {
            Ogre::SceneNode* hullRoot = mBoatRoot->createChildSceneNode(
                "hullRoot" + Ogre::StringConverter::toString(i), Ogre::Vector3(mHullBaseX[i], 0.0f, 0.0f));
            mHullNodes.push_back(hullRoot);

            hullRoot->attachObject(createTaperedHull(mScene, "hull" + Ogre::StringConverter::toString(i), "assoc/hullWhite"));

            Ogre::SceneNode* solarTop = hullRoot->createChildSceneNode(
                "solarTop" + Ogre::StringConverter::toString(i), Ogre::Vector3(0.0f, 5.0f, 0.0f));
            solarTop->attachObject(createBox(mScene, "solarTopBox" + Ogre::StringConverter::toString(i),
                Ogre::Vector3(58.0f, 0.45f, 7.0f), solarMats[i]));

            Ogre::SceneNode* battery = hullRoot->createChildSceneNode(
                "battery" + Ogre::StringConverter::toString(i), Ogre::Vector3(-11.0f, 3.0f, 0.0f));
            battery->attachObject(createBox(mScene, "batteryBox" + Ogre::StringConverter::toString(i),
                Ogre::Vector3(12.0f, 2.0f, 4.8f), "assoc/battery"));
        }

        for (const Ogre::Real z : {-14.0f, 14.0f})
        {
            Ogre::SceneNode* bridge = mBoatRoot->createChildSceneNode("bridge" + Ogre::StringConverter::toString(z),
                Ogre::Vector3(0.0f, 6.5f, z));
            bridge->attachObject(createBox(mScene, "bridgeBeam" + Ogre::StringConverter::toString(z),
                Ogre::Vector3(72.0f, 1.1f, 2.2f), "assoc/carbon"));

            Ogre::SceneNode* panel = mBoatRoot->createChildSceneNode("bridgeSolar" + Ogre::StringConverter::toString(z),
                Ogre::Vector3(0.0f, 7.25f, z));
            panel->attachObject(createBox(mScene, "bridgeSolarPanel" + Ogre::StringConverter::toString(z),
                Ogre::Vector3(62.0f, 0.35f, 3.0f), z < 0.0f ? "assoc/solarBlue" : "assoc/solarMint"));
        }

        for (const Ogre::Real x : {-14.0f, 14.0f})
        {
            Ogre::SceneNode* dock = mBoatRoot->createChildSceneNode("dockGlow" + Ogre::StringConverter::toString(x),
                Ogre::Vector3(x, 7.6f, 0.0f));
            dock->attachObject(createBox(mScene, "dockGlowBox" + Ogre::StringConverter::toString(x),
                Ogre::Vector3(2.4f, 0.7f, 44.0f), "assoc/dockGlow"));
        }

        mWakeNode = mScene->getRootSceneNode()->createChildSceneNode("wake", Ogre::Vector3(0.0f, kWaterY + 0.08f, 36.0f));
        mWakeNode->attachObject(createBox(mScene, "wakeFoam", Ogre::Vector3(92.0f, 0.08f, 95.0f), "assoc/wake"));
    }

    Ogre::RenderWindow* mRenderWindow{nullptr};
    Ogre::SceneManager* mScene{nullptr};
    Ogre::Camera* mCam{nullptr};
    Ogre::SceneNode* mCamNode{nullptr};

    Ogre::SceneNode* mWaterNode{nullptr};
    Ogre::SceneNode* mBoatRoot{nullptr};
    Ogre::SceneNode* mWakeNode{nullptr};
    std::vector<Ogre::SceneNode*> mHullNodes;
    std::array<Ogre::Real, 3> mHullBaseX{{-28.0f, 0.0f, 28.0f}};

    Ogre::Real mT{0.0f};
    Ogre::Real mSeparation{0.0f};
    bool mSeparated{false};
};

int main(int /*argc*/, char* /*argv*/[])
{
    try
    {
        preferX11WhenWaylandOgreIsUnavailable();
        AssocApp app;
        app.initApp();
        Ogre::Root* const root = app.getRoot();
        root->startRendering();
        app.closeApp();
    }
    catch (const Ogre::Exception& e)
    {
        Ogre::LogManager::getSingleton().logMessage(e.getFullDescription(), Ogre::LML_CRITICAL);
        return 1;
    }
    catch (const std::exception& e)
    {
        return 1;
    }
    return 0;
}
