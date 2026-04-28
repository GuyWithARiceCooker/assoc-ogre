/**
 * Meadow — Ogre3D Bites + RTShader: rét, szórt fák és bokrok (procedurális ManualObject).
 * Esc: kilépés.
 */

#include "Ogre.h"
#include "OgreApplicationContext.h"
#include "OgreInput.h"
#include "OgreRTShaderSystem.h"
#include "OgrePlane.h"
#include "OgreManualObject.h"
#include "OgreMaterialManager.h"
#include "OgreResourceGroupManager.h"

#include "AssocLinuxWaylandEnv.h"

#include <cmath>
#include <random>

namespace
{
    constexpr float kPi{3.14159265358979323846F};

    void makeSolidColourMaterial(Ogre::String const& name, Ogre::ColourValue const& ambient,
        Ogre::ColourValue const& diffuse)
    {
        Ogre::MaterialPtr const m{Ogre::MaterialManager::getSingleton().create(
            name, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME)};
        Ogre::Technique* const t{m->getTechnique(0)};
        Ogre::Pass* const p{t->getPass(0)};
        p->setAmbient(ambient);
        p->setDiffuse(diffuse);
        p->setSpecular(0.04F, 0.06F, 0.03F, 8.0F);
    }

    /** Y-felé álló henger, alja y=0. */
    Ogre::ManualObject* buildTrunk(Ogre::SceneManager* scene, Ogre::String const& name, float radius, float height,
        int segments)
    {
        Ogre::ManualObject* const mo{scene->createManualObject(name)};
        mo->begin("Meadow/Trunk", Ogre::RenderOperation::OT_TRIANGLE_LIST,
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

        for (int i{0}; i < segments; ++i)
        {
            float const a0{kPi * 2.0F * static_cast<float>(i) / static_cast<float>(segments)};
            float const a1{kPi * 2.0F * static_cast<float>(i + 1) / static_cast<float>(segments)};
            float const x0{radius * std::cos(a0)};
            float const z0{radius * std::sin(a0)};
            float const x1{radius * std::cos(a1)};
            float const z1{radius * std::sin(a1)};
            Ogre::Vector3 const n0{x0 / radius, 0.0F, z0 / radius};
            Ogre::Vector3 const n1{x1 / radius, 0.0F, z1 / radius};

            mo->position(x0, 0.0F, z0);
            mo->normal(n0);
            mo->position(x1, 0.0F, z1);
            mo->normal(n1);
            mo->position(x1, height, z1);
            mo->normal(n1);

            mo->position(x0, 0.0F, z0);
            mo->normal(n0);
            mo->position(x1, height, z1);
            mo->normal(n1);
            mo->position(x0, height, z0);
            mo->normal(n0);
        }
        mo->end();
        return mo;
    }

    /** Kúp: alap y=0 sugarú, csúcs y=height. */
    Ogre::ManualObject* buildCone(Ogre::SceneManager* scene, Ogre::String const& name, float baseRadius, float height,
        int segments, Ogre::String const& materialName)
    {
        Ogre::ManualObject* const mo{scene->createManualObject(name)};
        mo->begin(materialName, Ogre::RenderOperation::OT_TRIANGLE_LIST,
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

        float const apexY{height};
        for (int i{0}; i < segments; ++i)
        {
            float const a0{kPi * 2.0F * static_cast<float>(i) / static_cast<float>(segments)};
            float const a1{kPi * 2.0F * static_cast<float>(i + 1) / static_cast<float>(segments)};
            float const x0{baseRadius * std::cos(a0)};
            float const z0{baseRadius * std::sin(a0)};
            float const x1{baseRadius * std::cos(a1)};
            float const z1{baseRadius * std::sin(a1)};
            Ogre::Vector3 const e0{x0, 0.0F, z0};
            Ogre::Vector3 const e1{x1, 0.0F, z1};
            Ogre::Vector3 const tip{0.0F, apexY, 0.0F};
            Ogre::Vector3 const n{(e0 + e1 + tip) / 3.0F - Ogre::Vector3(0.0F, apexY * 0.33F, 0.0F)};
            Ogre::Vector3 nn{n.normalisedCopy()};

            mo->position(tip.x, tip.y, tip.z);
            mo->normal(nn);
            mo->position(e0.x, e0.y, e0.z);
            mo->normal(nn);
            mo->position(e1.x, e1.y, e1.z);
            mo->normal(nn);
        }

        // alsó korong (látható alulról / árnyék)
        Ogre::Vector3 const down{0.0F, -1.0F, 0.0F};
        for (int i{0}; i < segments; ++i)
        {
            float const a0{kPi * 2.0F * static_cast<float>(i) / static_cast<float>(segments)};
            float const a1{kPi * 2.0F * static_cast<float>(i + 1) / static_cast<float>(segments)};
            float const x0{baseRadius * std::cos(a0)};
            float const z0{baseRadius * std::sin(a0)};
            float const x1{baseRadius * std::cos(a1)};
            float const z1{baseRadius * std::sin(a1)};
            mo->position(0.0F, 0.0F, 0.0F);
            mo->normal(down);
            mo->position(x1, 0.0F, z1);
            mo->normal(down);
            mo->position(x0, 0.0F, z0);
            mo->normal(down);
        }
        mo->end();
        return mo;
    }

    /** Egyszerű gömb (szögletes stack), középpont az origóban. */
    Ogre::ManualObject* buildSphere(Ogre::SceneManager* scene, Ogre::String const& name, float radius, int slices,
        int stacks, Ogre::String const& materialName)
    {
        Ogre::ManualObject* const mo{scene->createManualObject(name)};
        mo->begin(materialName, Ogre::RenderOperation::OT_TRIANGLE_LIST,
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

        for (int stack{0}; stack < stacks; ++stack)
        {
            float const phi0{kPi * static_cast<float>(stack) / static_cast<float>(stacks)};
            float const phi1{kPi * static_cast<float>(stack + 1) / static_cast<float>(stacks)};
            float const y0{radius * std::cos(phi0)};
            float const y1{radius * std::cos(phi1)};
            float const r0{radius * std::sin(phi0)};
            float const r1{radius * std::sin(phi1)};

            for (int slice{0}; slice < slices; ++slice)
            {
                float const t0{kPi * 2.0F * static_cast<float>(slice) / static_cast<float>(slices)};
                float const t1{kPi * 2.0F * static_cast<float>(slice + 1) / static_cast<float>(slices)};
                float const x00{r0 * std::cos(t0)};
                float const z00{r0 * std::sin(t0)};
                float const x01{r0 * std::cos(t1)};
                float const z01{r0 * std::sin(t1)};
                float const x10{r1 * std::cos(t0)};
                float const z10{r1 * std::sin(t0)};
                float const x11{r1 * std::cos(t1)};
                float const z11{r1 * std::sin(t1)};

                Ogre::Vector3 v000{x00, y0, z00};
                Ogre::Vector3 v010{x01, y0, z01};
                Ogre::Vector3 v101{x11, y1, z11};
                Ogre::Vector3 v110{x10, y1, z10};
                Ogre::Vector3 n0{v000.normalisedCopy()};
                Ogre::Vector3 n1{v010.normalisedCopy()};
                Ogre::Vector3 n2{v101.normalisedCopy()};
                Ogre::Vector3 n3{v110.normalisedCopy()};

                mo->position(v000.x, v000.y, v000.z);
                mo->normal(n0);
                mo->position(v010.x, v010.y, v010.z);
                mo->normal(n1);
                mo->position(v101.x, v101.y, v101.z);
                mo->normal(n2);

                mo->position(v000.x, v000.y, v000.z);
                mo->normal(n0);
                mo->position(v101.x, v101.y, v101.z);
                mo->normal(n2);
                mo->position(v110.x, v110.y, v110.z);
                mo->normal(n3);
            }
        }
        mo->end();
        return mo;
    }
} // namespace

class MeadowApp
    : public OgreBites::ApplicationContext
    , public OgreBites::InputListener
{
public:
    MeadowApp()
        : OgreBites::ApplicationContext("meadow")
    {
    }

    void setup() override
    {
        OgreBites::ApplicationContext::setup();
        addInputListener(this);

        makeSolidColourMaterial("Meadow/Ground", Ogre::ColourValue(0.12F, 0.28F, 0.1F),
            Ogre::ColourValue(0.28F, 0.62F, 0.22F));
        makeSolidColourMaterial("Meadow/Trunk", Ogre::ColourValue(0.18F, 0.12F, 0.06F),
            Ogre::ColourValue(0.38F, 0.26F, 0.14F));
        makeSolidColourMaterial("Meadow/Foliage", Ogre::ColourValue(0.06F, 0.14F, 0.05F),
            Ogre::ColourValue(0.14F, 0.42F, 0.12F));
        makeSolidColourMaterial("Meadow/FoliageLight", Ogre::ColourValue(0.08F, 0.18F, 0.06F),
            Ogre::ColourValue(0.22F, 0.55F, 0.18F));
        makeSolidColourMaterial("Meadow/Bush", Ogre::ColourValue(0.08F, 0.2F, 0.07F),
            Ogre::ColourValue(0.2F, 0.48F, 0.16F));

        Ogre::Root* const root{getRoot()};
        mScene = root->createSceneManager();
        Ogre::RTShader::ShaderGenerator::getSingleton().addSceneManager(mScene);

        mScene->setAmbientLight(Ogre::ColourValue(0.22F, 0.28F, 0.35F));

        Ogre::Light* sun{mScene->createLight("sun")};
        sun->setType(Ogre::Light::LT_DIRECTIONAL);
        Ogre::SceneNode* sunNode{mScene->getRootSceneNode()->createChildSceneNode("sunNode")};
        sunNode->setDirection(Ogre::Vector3(-0.35F, -0.82F, -0.28F).normalisedCopy());
        sunNode->attachObject(sun);
        sun->setDiffuseColour(1.0F, 0.96F, 0.88F);

        Ogre::Light* hemi{mScene->createLight("hemi")};
        hemi->setType(Ogre::Light::LT_DIRECTIONAL);
        Ogre::SceneNode* hemiNode{mScene->getRootSceneNode()->createChildSceneNode("hemiNode")};
        hemiNode->setDirection(Ogre::Vector3(0.2F, 0.75F, 0.35F).normalisedCopy());
        hemiNode->attachObject(hemi);
        hemi->setDiffuseColour(0.22F, 0.28F, 0.38F);

        mCamNode = mScene->getRootSceneNode()->createChildSceneNode("cam");
        mCamNode->setPosition(0.0F, 42.0F, 168.0F);
        mCam = mScene->createCamera("main");
        mCam->setNearClipDistance(0.5f);
        mCam->setFarClipDistance(2500.0F);
        mCam->setAutoAspectRatio(true);
        mCamNode->attachObject(mCam);

        mRenderWindow = getRenderWindow();
        mScene->setFog(Ogre::FOG_LINEAR, Ogre::ColourValue(0.55F, 0.68F, 0.82F), 0.0F, 120.0F, 520.0F);
        {
            Ogre::Viewport* const vp{mRenderWindow->addViewport(mCam)};
            vp->setBackgroundColour(Ogre::ColourValue(0.52F, 0.72F, 0.88F));
            vp->setClearEveryFrame(true);
        }

        float const field{900.0F};
        Ogre::MeshManager::getSingleton().createPlane(
            "meadowPlane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::Plane(Ogre::Vector3::UNIT_Y, 0.0F), field, field, 48, 48, true, 1, 24.0F, 24.0F,
            Ogre::Vector3::UNIT_Z);

        Ogre::Entity* const ground{mScene->createEntity("meadowPlane")};
        mGroundNode = mScene->getRootSceneNode()->createChildSceneNode("ground");
        mGroundNode->attachObject(ground);
        ground->setMaterialName("Meadow/Ground");
        mGroundNode->pitch(Ogre::Degree(180.0F));

        std::mt19937 rng{std::random_device{}()};
        std::uniform_real_distribution<float> xy{-field * 0.46F, field * 0.46F};
        std::uniform_real_distribution<float> yaw{0.0F, kPi * 2.0F};
        std::uniform_real_distribution<float> scaleTrunk{0.65F, 1.25F};
        std::uniform_real_distribution<float> crownScale{0.75F, 1.15F};

        int treeId{0};
        for (int n{0}; n < 95; ++n)
        {
            float const x{xy(rng)};
            float const z{xy(rng)};
            float const sy{scaleTrunk(rng)};
            float const trunkR{0.35F * sy};
            float const trunkH{5.2F * sy};
            float const crownR{2.1F * sy * crownScale(rng)};
            float const crownH{6.5F * sy * crownScale(rng)};

            Ogre::SceneNode* const root{mScene->getRootSceneNode()->createChildSceneNode(
                "tree_" + Ogre::StringConverter::toString(treeId), Ogre::Vector3(x, 0.0F, z))};
            root->yaw(Ogre::Radian(yaw(rng)));

            Ogre::ManualObject* const trunkMo{
                buildTrunk(mScene, "trunk_" + Ogre::StringConverter::toString(treeId), trunkR, trunkH, 10)};
            root->attachObject(trunkMo);

            Ogre::SceneNode* const crownNode{root->createChildSceneNode("crown_" + Ogre::StringConverter::toString(treeId))};
            crownNode->setPosition(0.0F, trunkH * 0.92F, 0.0F);
            Ogre::ManualObject* const lower{
                buildCone(mScene, "coneL_" + Ogre::StringConverter::toString(treeId), crownR * 1.08F, crownH * 0.55F, 12,
                    "Meadow/Foliage")};
            crownNode->attachObject(lower);

            Ogre::SceneNode* const upper{crownNode->createChildSceneNode("upper_" + Ogre::StringConverter::toString(treeId))};
            upper->setPosition(0.0F, crownH * 0.38F, 0.0F);
            Ogre::ManualObject* const topCone{
                buildCone(mScene, "coneU_" + Ogre::StringConverter::toString(treeId), crownR * 0.72F, crownH * 0.48F, 10,
                    "Meadow/FoliageLight")};
            upper->attachObject(topCone);

            ++treeId;
        }

        int bushId{0};
        std::uniform_real_distribution<float> bushR{0.55F, 1.35F};
        for (int n{0}; n < 220; ++n)
        {
            float const x{xy(rng)};
            float const z{xy(rng)};
            float const br{bushR(rng)};
            Ogre::SceneNode* const bn{mScene->getRootSceneNode()->createChildSceneNode(
                "bush_" + Ogre::StringConverter::toString(bushId), Ogre::Vector3(x, br * 0.85F, z))};
            bn->yaw(Ogre::Radian(yaw(rng)));
            bn->setScale(br, br * 0.78F, br);
            Ogre::ManualObject* const bush{
                buildSphere(mScene, "bushS_" + Ogre::StringConverter::toString(bushId), 1.0F, 10, 6, "Meadow/Bush")};
            bn->attachObject(bush);
            ++bushId;
        }

        mCamNode->lookAt(Ogre::Vector3(0.0F, 12.0F, 0.0F), Ogre::Node::TS_WORLD, Ogre::Vector3::NEGATIVE_UNIT_Z);
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
        if (mCamNode)
        {
            float const orbit{115.0F};
            float const ang{mT * 0.045F};
            float const cx{orbit * std::cos(ang)};
            float const cz{orbit * std::sin(ang)};
            mCamNode->setPosition(cx, 46.0F + 6.0F * std::sin(mT * 0.08F), cz);
            mCamNode->lookAt(Ogre::Vector3(0.0F, 10.0F, 0.0F), Ogre::Node::TS_WORLD, Ogre::Vector3::NEGATIVE_UNIT_Z);
        }
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
    Ogre::RenderWindow* mRenderWindow{nullptr};
    Ogre::SceneManager* mScene{nullptr};
    Ogre::Camera* mCam{nullptr};
    Ogre::SceneNode* mCamNode{nullptr};
    Ogre::SceneNode* mGroundNode{nullptr};
    Ogre::Real mT{0.0f};
};

int main(int /*argc*/, char* /*argv*/[])
{
    AssocLinuxEnv::preferX11ForDistroOgreOnWayland();
    try
    {
        MeadowApp app;
        app.initApp();
        Ogre::Root* const root{app.getRoot()};
        root->startRendering();
        app.closeApp();
    }
    catch (const Ogre::Exception& e)
    {
        Ogre::LogManager::getSingleton().logMessage(e.getFullDescription(), Ogre::LML_CRITICAL);
        return 1;
    }
    catch (const std::exception&)
    {
        return 1;
    }
    return 0;
}
