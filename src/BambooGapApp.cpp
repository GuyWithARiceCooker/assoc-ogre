/**
 * bamboo_gap — ~90 „cm” nyílás kitöltése bambusz félhengerekkel (ManualObject).
 * Tab / ] következő elrendezés, [ előző, Esc kilépés.
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
#include <vector>

namespace
{
constexpr float kPi{3.14159265358979323846F};
/** Nyílás szélessége (ugyanaz a skála mint a „90 cm” koncepció). */
constexpr float kGapHalf{45.0F};

void makeMat(Ogre::String const& name, Ogre::ColourValue const& amb, Ogre::ColourValue const& diff)
{
    Ogre::MaterialPtr const m{Ogre::MaterialManager::getSingleton().create(
        name, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME)};
    Ogre::Pass* const p{m->getTechnique(0)->getPass(0)};
    p->setAmbient(amb);
    p->setDiffuse(diff);
    p->setSpecular(0.06F, 0.08F, 0.04F, 12.0F);
}

/**
 * Függőleges félhenger: Y a magasság, keresztmetszet az XZ síkon félkör +Z felé,
 * sík vágás z=0 síkon (belül a rés felé).
 */
Ogre::ManualObject* buildVerticalHalfCylinder(Ogre::SceneManager* scene, Ogre::String const& baseName,
    float radius, float height, int arcSeg, int heightSeg, Ogre::String const& materialName)
{
    Ogre::ManualObject* const mo{scene->createManualObject(baseName)};
    mo->begin(materialName, Ogre::RenderOperation::OT_TRIANGLE_LIST,
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

    auto pushQuad = [&](Ogre::Vector3 const& a, Ogre::Vector3 const& b, Ogre::Vector3 const& c,
                      Ogre::Vector3 const& d, Ogre::Vector3 const& n) {
        mo->position(a);
        mo->normal(n);
        mo->position(b);
        mo->normal(n);
        mo->position(c);
        mo->normal(n);
        mo->position(a);
        mo->normal(n);
        mo->position(c);
        mo->normal(n);
        mo->position(d);
        mo->normal(n);
    };

    // Ívelt palást: φ ∈ [0, π] → x=R cos φ, z=R sin φ (félkör +Z irányba)
    for (int i{0}; i < arcSeg; ++i)
    {
        float const t0{kPi * static_cast<float>(i) / static_cast<float>(arcSeg)};
        float const t1{kPi * static_cast<float>(i + 1) / static_cast<float>(arcSeg)};
        for (int j{0}; j < heightSeg; ++j)
        {
            float const y0{height * static_cast<float>(j) / static_cast<float>(heightSeg)};
            float const y1{height * static_cast<float>(j + 1) / static_cast<float>(heightSeg)};
            float const x0{radius * std::cos(t0)};
            float const z0{radius * std::sin(t0)};
            float const x1{radius * std::cos(t1)};
            float const z1{radius * std::sin(t1)};
            Ogre::Vector3 const n0{std::cos(t0), 0.0F, std::sin(t0)};
            Ogre::Vector3 const n1{std::cos(t1), 0.0F, std::sin(t1)};
            Ogre::Vector3 const n{(n0 + n1).normalisedCopy()};
            pushQuad({x0, y0, z0}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z0}, n);
        }
    }

    // Sík vágás (z≈0): háromszögek a húron |x|≤R
    Ogre::Vector3 const flatN{0.0F, 0.0F, -1.0F};
    for (int j{0}; j < heightSeg; ++j)
    {
        float const y0{height * static_cast<float>(j) / static_cast<float>(heightSeg)};
        float const y1{height * static_cast<float>(j + 1) / static_cast<float>(heightSeg)};
        mo->position(-radius, y0, 0.0F);
        mo->normal(flatN);
        mo->position(radius, y0, 0.0F);
        mo->normal(flatN);
        mo->position(radius, y1, 0.0F);
        mo->normal(flatN);
        mo->position(-radius, y0, 0.0F);
        mo->normal(flatN);
        mo->position(radius, y1, 0.0F);
        mo->normal(flatN);
        mo->position(-radius, y1, 0.0F);
        mo->normal(flatN);
    }

    mo->end();
    return mo;
}

enum class LayoutMode : int
{
    Lamella = 0,
    DoubleRow,
    Grid,
    Wave,
    Herringbone,
    Arc,
    Sparse,
    Count
};

char const* layoutName(LayoutMode m)
{
    switch (m)
    {
    case LayoutMode::Lamella:
        return "Lamella (egy sor)";
    case LayoutMode::DoubleRow:
        return "Ket sor Z-ban";
    case LayoutMode::Grid:
        return "Racs";
    case LayoutMode::Wave:
        return "Hullam (forgatas)";
    case LayoutMode::Herringbone:
        return "Valszo";
    case LayoutMode::Arc:
        return "Iv (magassag hullam)";
    case LayoutMode::Sparse:
        return "Szort szerves";
    default:
        return "?";
    }
}

} // namespace

class BambooGapApp
    : public OgreBites::ApplicationContext
    , public OgreBites::InputListener
{
public:
    BambooGapApp()
        : OgreBites::ApplicationContext("bamboo_gap")
    {
    }

    void setup() override
    {
        OgreBites::ApplicationContext::setup();
        addInputListener(this);

        makeMat("Bamboo/Stem", Ogre::ColourValue(0.12F, 0.22F, 0.08F),
            Ogre::ColourValue(0.35F, 0.52F, 0.2F));
        makeMat("Bamboo/StemDark", Ogre::ColourValue(0.08F, 0.14F, 0.05F),
            Ogre::ColourValue(0.22F, 0.36F, 0.14F));
        makeMat("Bamboo/Wall", Ogre::ColourValue(0.18F, 0.16F, 0.14F),
            Ogre::ColourValue(0.42F, 0.38F, 0.34F));
        makeMat("Bamboo/Floor", Ogre::ColourValue(0.1F, 0.12F, 0.09F),
            Ogre::ColourValue(0.28F, 0.32F, 0.24F));

        Ogre::Root* const root{getRoot()};
        mScene = root->createSceneManager();
        Ogre::RTShader::ShaderGenerator::getSingleton().addSceneManager(mScene);

        mScene->setAmbientLight(Ogre::ColourValue(0.28F, 0.32F, 0.38F));

        Ogre::Light* sun{mScene->createLight("sun")};
        sun->setType(Ogre::Light::LT_DIRECTIONAL);
        Ogre::SceneNode* sn{mScene->getRootSceneNode()->createChildSceneNode("sun")};
        sn->setDirection(Ogre::Vector3(-0.4F, -0.85F, -0.25F).normalisedCopy());
        sn->attachObject(sun);
        sun->setDiffuseColour(1.0F, 0.98F, 0.92F);

        mCamNode = mScene->getRootSceneNode()->createChildSceneNode("cam");
        mCamNode->setPosition(0.0F, 95.0F, 220.0F);
        mCam = mScene->createCamera("main");
        mCam->setNearClipDistance(0.5f);
        mCam->setFarClipDistance(2000.0F);
        mCam->setAutoAspectRatio(true);
        mCamNode->attachObject(mCam);

        mRenderWindow = getRenderWindow();
        mScene->setFog(Ogre::FOG_LINEAR, Ogre::ColourValue(0.55F, 0.62F, 0.72F), 0.0F, 80.0F, 420.0F);
        {
            Ogre::Viewport* const vp{mRenderWindow->addViewport(mCam)};
            vp->setBackgroundColour(Ogre::ColourValue(0.5F, 0.62F, 0.78F));
        }

        Ogre::MeshManager::getSingleton().createPlane(
            "bambooFloor", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::Plane(Ogre::Vector3::UNIT_Y, 0.0F), 320.0F, 240.0F, 4, 4, true, 1, 1.0F, 1.0F,
            Ogre::Vector3::UNIT_Z);
        Ogre::Entity* const fl{mScene->createEntity("bambooFloor")};
        Ogre::SceneNode* const fn{mScene->getRootSceneNode()->createChildSceneNode("floor")};
        fn->attachObject(fl);
        fl->setMaterialName("Bamboo/Floor");
        // +Y normál marad — pitch(180) hátlapot mutatott volna felülről.

        // „Falak” a nyílás szélén (vizuális 90 cm köz)
        buildWallPlane("wL", Ogre::Vector3(-kGapHalf, 75.0F, 0.0F), Ogre::Vector3::UNIT_X);
        buildWallPlane("wR", Ogre::Vector3(kGapHalf, 75.0F, 0.0F), Ogre::Vector3::NEGATIVE_UNIT_X);

        mLayoutRoot = mScene->getRootSceneNode()->createChildSceneNode("bambooLayouts");
        rebuildLayout();

        mCamNode->lookAt(Ogre::Vector3(0.0F, 55.0F, 0.0F), Ogre::Node::TS_WORLD, Ogre::Vector3::NEGATIVE_UNIT_Z);
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
            float const a{mT * 0.12F};
            float const x{35.0F * std::sin(a)};
            float const z{145.0F + 45.0F * std::cos(a * 0.7F)};
            mCamNode->setPosition(x, 88.0F + 10.0F * std::sin(mT * 0.08F), z);
            mCamNode->lookAt(Ogre::Vector3(0.0F, 50.0F, 0.0F), Ogre::Node::TS_WORLD, Ogre::Vector3::NEGATIVE_UNIT_Z);
        }
        return OgreBites::ApplicationContext::frameRenderingQueued(evt);
    }

    bool keyPressed(const OgreBites::KeyboardEvent& key) override
    {
        if (key.keysym.sym == OgreBites::SDLK_ESCAPE)
        {
            Ogre::Root::getSingleton().queueEndRendering();
            return true;
        }
        if (key.keysym.sym == static_cast<int>('\t') || key.keysym.sym == static_cast<int>(']'))
        {
            nextLayout(1);
            return true;
        }
        if (key.keysym.sym == static_cast<int>('['))
        {
            nextLayout(-1);
            return true;
        }
        return true;
    }

private:
    void buildWallPlane(Ogre::String const& id, Ogre::Vector3 const& center, Ogre::Vector3 const& normalHint)
    {
        float const w{40.0F};
        float const h{150.0F};
        Ogre::ManualObject* const mo{mScene->createManualObject("wall_" + id)};
        mo->begin("Bamboo/Wall", Ogre::RenderOperation::OT_TRIANGLE_LIST,
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        Ogre::Vector3 const n{normalHint.normalisedCopy()};
        Ogre::Vector3 const up{Ogre::Vector3::UNIT_Y * (h * 0.5F)};
        Ogre::Vector3 const side{Ogre::Vector3::UNIT_Z.crossProduct(n).normalisedCopy() * (w * 0.5F)};
        Ogre::Vector3 const c{center};
        Ogre::Vector3 const v00{c - side - up};
        Ogre::Vector3 const v01{c + side - up};
        Ogre::Vector3 const v11{c + side + up};
        Ogre::Vector3 const v10{c - side + up};
        mo->position(v00);
        mo->normal(n);
        mo->position(v01);
        mo->normal(n);
        mo->position(v11);
        mo->normal(n);
        mo->position(v00);
        mo->normal(n);
        mo->position(v11);
        mo->normal(n);
        mo->position(v10);
        mo->normal(n);
        mo->end();
        Ogre::SceneNode* const wn{mScene->getRootSceneNode()->createChildSceneNode("wn_" + id, c)};
        wn->attachObject(mo);
    }

    void clearLayoutNodes()
    {
        if (!mLayoutRoot)
        {
            return;
        }
        while (mLayoutRoot->numChildren() > 0)
        {
            Ogre::SceneNode* const ch{static_cast<Ogre::SceneNode*>(mLayoutRoot->getChild(0))};
            while (ch->numAttachedObjects() > 0)
            {
                Ogre::MovableObject* const o{ch->getAttachedObject(0)};
                ch->detachObject(o);
                Ogre::ManualObject* const mo{dynamic_cast<Ogre::ManualObject*>(o)};
                if (mo)
                {
                    mScene->destroyManualObject(mo);
                }
            }
            mLayoutRoot->removeChild(ch);
            mScene->destroySceneNode(ch);
        }
    }

    void placePiece(Ogre::String const& id, Ogre::Vector3 const& pos, float yawDeg, float scaleY,
        Ogre::String const& mat)
    {
        float const R{4.2F};
        float const H{78.0F * scaleY};
        Ogre::ManualObject* mo{
            buildVerticalHalfCylinder(mScene, "bamboo_" + id, R, H, 10, 6, mat)};
        Ogre::SceneNode* const n{mLayoutRoot->createChildSceneNode("node_" + id, pos)};
        n->yaw(Ogre::Degree(yawDeg));
        n->attachObject(mo);
    }

    void rebuildLayout()
    {
        clearLayoutNodes();
        LayoutMode const mode{static_cast<LayoutMode>(mModeIdx)};
        float const innerL{-kGapHalf + 5.0F};
        float const innerR{kGapHalf - 5.0F};

        switch (mode)
        {
        case LayoutMode::Lamella:
        {
            int k{0};
            for (float x{innerL + 3.0F}; x < innerR; x += 9.5F)
            {
                placePiece(Ogre::StringConverter::toString(k++), {x, 0.0F, 0.0F}, 0.0F, 1.0F, "Bamboo/Stem");
            }
            break;
        }
        case LayoutMode::DoubleRow:
        {
            int k{0};
            for (float x{innerL + 4.0F}; x < innerR; x += 10.0F)
            {
                placePiece(Ogre::StringConverter::toString(k++), {x, 0.0F, -18.0F}, 0.0F, 1.0F, "Bamboo/Stem");
                placePiece(Ogre::StringConverter::toString(k++), {x, 0.0F, 18.0F}, 180.0F, 1.0F, "Bamboo/StemDark");
            }
            break;
        }
        case LayoutMode::Grid:
        {
            int k{0};
            for (float x{innerL + 5.0F}; x < innerR; x += 11.0F)
            {
                for (float z{-28.0F}; z <= 28.0F; z += 18.0F)
                {
                    float const yaw{(k % 2) == 0 ? 0.0F : 180.0F};
                    char const* mat{(k % 3) == 0 ? "Bamboo/StemDark" : "Bamboo/Stem"};
                    placePiece(Ogre::StringConverter::toString(k++), {x, 0.0F, z}, yaw, 0.92F, mat);
                }
            }
            break;
        }
        case LayoutMode::Wave:
        {
            int k{0};
            for (float x{innerL + 4.0F}; x < innerR; x += 9.0F)
            {
                float const z{22.0F * std::sin(x * 0.09F)};
                float const yaw{25.0F * std::cos(x * 0.11F)};
                placePiece(Ogre::StringConverter::toString(k++), {x, 0.0F, z}, yaw, 1.0F, "Bamboo/Stem");
            }
            break;
        }
        case LayoutMode::Herringbone:
        {
            int k{0};
            for (float x{innerL + 5.0F}; x < innerR; x += 12.0F)
            {
                float const yaw{((k % 2) == 0) ? 55.0F : -55.0F};
                placePiece(Ogre::StringConverter::toString(k++), {x, 0.0F, 0.0F}, yaw, 1.05F, "Bamboo/StemDark");
            }
            break;
        }
        case LayoutMode::Arc:
        {
            int k{0};
            for (float x{innerL + 4.0F}; x < innerR; x += 10.0F)
            {
                float const cx{(x - innerL) / (innerR - innerL) - 0.5F};
                float const sy{0.65F + 0.55F * std::cos(cx * kPi)};
                placePiece(Ogre::StringConverter::toString(k++), {x, 0.0F, 0.0F}, 0.0F, sy, "Bamboo/Stem");
            }
            break;
        }
        case LayoutMode::Sparse:
        {
            int k{0};
            float x{innerL + 6.0F};
            while (x < innerR)
            {
                float const z{15.0F * std::sin(x * 0.17F + 1.3F)};
                placePiece(Ogre::StringConverter::toString(k++), {x, 0.0F, z},
                    12.0F * std::sin(x * 0.2F), 0.85F + 0.15F * std::sin(x * 0.31F), "Bamboo/Stem");
                x += 14.0F + 6.0F * std::sin(x * 0.08F);
            }
            break;
        }
        default:
            break;
        }
        logLayout();
    }

    void nextLayout(int delta)
    {
        int const n{static_cast<int>(LayoutMode::Count)};
        mModeIdx = ((mModeIdx + delta) % n + n) % n;
        rebuildLayout();
    }

    void logLayout()
    {
        Ogre::String const msg{Ogre::String("[bamboo_gap] ") + layoutName(static_cast<LayoutMode>(mModeIdx))
            + "  |  [ ] elozo/kovetkezo  Tab valtas  Esc"};
        Ogre::LogManager::getSingleton().logMessage(msg);
    }

    Ogre::RenderWindow* mRenderWindow{nullptr};
    Ogre::SceneManager* mScene{nullptr};
    Ogre::Camera* mCam{nullptr};
    Ogre::SceneNode* mCamNode{nullptr};
    Ogre::SceneNode* mLayoutRoot{nullptr};
    int mModeIdx{0};
    Ogre::Real mT{0.0f};
};

int main(int /*argc*/, char* /*argv*/[])
{
    AssocLinuxEnv::applyLinuxDisplayEnvForOgre();
    try
    {
        BambooGapApp app;
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
