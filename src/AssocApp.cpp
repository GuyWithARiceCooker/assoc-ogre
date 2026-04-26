/**
 * ============================================================================
 * assoc (AssocApp.cpp) — Ogre3D 14 + Bites + RTShader: 3D jelenet, szöveg **nem** overlay,
 * hanem a jelenetben: betűnként külön `ManualObject` (quad + font-UV, SdkTrays/Caption ttf)
 * ============================================================================
 *
 * FŐ ADATFOLYAM
 * -------------
 * `initApp` → `locateResources` / `loadResources` (Essential: SdkTrays.zip → font) →
 * `setup()`: SceneManager, RTSS, 3D szöveg: `TVC_DIFFUSE` + arany `ManualObject::colour`, TUS nearest+clamp,
 * `setAutoTracking` a kamera csomópontra, hogy a felirat a kamerába nézzen.
 * Esc → kilépés.
 * ============================================================================
 */

#include "Ogre.h"
#include "OgreApplicationContext.h"
#include "OgreInput.h"
#include "OgreRTShaderSystem.h"
#include "OgrePlane.h"
#include "OgreSubEntity.h"
#include "OgreTechnique.h"
#include "OgreFont.h"
#include "OgreFontManager.h"
#include "OgreManualObject.h"
#include "OgreMovableObject.h"
#include "OgreResourceGroupManager.h"
#include "OgreTextureUnitState.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace
{
    /** A `OgreBites::TextAreaOverlayElement` lépésképlete (overlay), világ-méretre `h` magassággal. */
    Ogre::Real measure3DLineWidth(Ogre::Font& font, const std::vector<std::uint32_t>& cps, Ogre::Real h)
    {
        Ogre::Real cursor{0.0F};
        for (std::uint32_t const code : cps)
        {
            if (code == static_cast<std::uint32_t>(' '))
            {
                cursor += font.getGlyphInfo(code).advance * h;
                continue;
            }
            Ogre::GlyphInfo const& g = font.getGlyphInfo(code);
            if (g.uvRect.isNull())
            {
                cursor += (g.advance - g.bearing) * h;
                continue;
            }
            cursor += g.advance * h;
        }
        return cursor;
    }

    /**
     * Egy sorbetűnként: külön `ManualObject` (2× háromszög) + `SceneNode` a szülő alatt.
     * Lokal: X jobbra, Y felfelé, Z=0; a kamera felé a szülő `setAutoTracking` forgatja a sort.
     */
    /** Arany tónus a glyph textúrával; minden csúcson `colour`, különben (0,0,0) diffúz → fekete. */
    void add3DTextLine(Ogre::SceneManager* const scene, Ogre::SceneNode* const lineRoot,
        Ogre::String const& namePrefix, Ogre::Font& font, Ogre::Material const& textMat, Ogre::String const& lineUtf8,
        Ogre::Real const emHeight, Ogre::uint32& nextId)
    {
        font.load();
        std::vector<std::uint32_t> const cps{Ogre::utftoc32(lineUtf8)};
        Ogre::Real const halfWidth = measure3DLineWidth(font, cps, emHeight) * 0.5F;
        Ogre::Real cursorX{-halfWidth};

        for (std::uint32_t const code : cps)
        {
            if (code == static_cast<std::uint32_t>('\n'))
            {
                break;
            }
            if (code == static_cast<std::uint32_t>(' '))
            {
                cursorX += font.getGlyphInfo(code).advance * emHeight;
                continue;
            }

            Ogre::GlyphInfo const& g = font.getGlyphInfo(code);
            if (g.uvRect.isNull())
            {
                cursorX += (g.advance - g.bearing) * emHeight;
                continue;
            }

            Ogre::Real const w = g.aspectRatio * emHeight;
            Ogre::FloatRect const& uv{g.uvRect};
            Ogre::Real const x0{cursorX + g.bearing * emHeight};

            Ogre::ManualObject* const mo{scene->createManualObject(namePrefix + Ogre::StringConverter::toString(nextId))};
            ++nextId;
            mo->begin(
                textMat.getName(), Ogre::RenderOperation::OT_TRIANGLE_LIST, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            Ogre::Real const yTop{0.0F};
            Ogre::Real const yBot{yTop - emHeight};
            // `position` → `colour` → `uv`: TVC_DIFFUSE; a +Z oldal, szülő `setAutoTracking` forgat.
            Ogre::ColourValue const gold{0.99f, 0.86f, 0.2f, 1.0f};
            mo->position(x0, yTop, 0.0F);
            mo->colour(gold);
            mo->textureCoord(uv.left, uv.top);
            mo->position(x0, yBot, 0.0F);
            mo->colour(gold);
            mo->textureCoord(uv.left, uv.bottom);
            mo->position(x0 + w, yTop, 0.0F);
            mo->colour(gold);
            mo->textureCoord(uv.right, uv.top);
            mo->position(x0 + w, yTop, 0.0F);
            mo->colour(gold);
            mo->textureCoord(uv.right, uv.top);
            mo->position(x0, yBot, 0.0F);
            mo->colour(gold);
            mo->textureCoord(uv.left, uv.bottom);
            mo->position(x0 + w, yBot, 0.0F);
            mo->colour(gold);
            mo->textureCoord(uv.right, uv.bottom);
            mo->end();
            Ogre::SceneNode* const letterNode{lineRoot->createChildSceneNode(
                namePrefix + "node_" + Ogre::StringConverter::toString(nextId - 1))};
            letterNode->attachObject(mo);
            cursorX += (g.advance - g.bearing) * emHeight;
        }
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

    void setup() override
    {
        OgreBites::ApplicationContext::setup();
        addInputListener(this);

        Ogre::Root* const root{getRoot()};
        mScene = root->createSceneManager();
        Ogre::RTShader::ShaderGenerator::getSingleton().addSceneManager(mScene);

        mScene->setAmbientLight(Ogre::ColourValue(0.1f, 0.12f, 0.18f));

        Ogre::Light* key = mScene->createLight("key");
        key->setType(Ogre::Light::LT_DIRECTIONAL);
        Ogre::SceneNode* kNode = mScene->getRootSceneNode()->createChildSceneNode("keyNode");
        kNode->setDirection(Ogre::Vector3(-0.2f, -0.7f, -0.4f).normalisedCopy());
        kNode->attachObject(key);
        key->setDiffuseColour(1.0f, 0.97f, 0.9f);

        Ogre::Light* fill = mScene->createLight("fill");
        fill->setType(Ogre::Light::LT_POINT);
        Ogre::SceneNode* fNode = mScene->getRootSceneNode()->createChildSceneNode("fillNode",
            Ogre::Vector3(-90.0f, 55.0f, 60.0f));
        fNode->attachObject(fill);
        fill->setDiffuseColour(0.35f, 0.45f, 0.7f);
        fill->setAttenuation(250.0f, 1.0f, 0.007f, 0.0f);

        Ogre::Light* rim = mScene->createLight("rim");
        rim->setType(Ogre::Light::LT_SPOTLIGHT);
        Ogre::SceneNode* rNode = mScene->getRootSceneNode()->createChildSceneNode("rimNode",
            Ogre::Vector3(100.0f, 95.0f, -20.0f));
        rNode->lookAt(Ogre::Vector3(0.0f, 25.0f, 0.0f), Ogre::Node::TS_PARENT);
        rNode->attachObject(rim);
        rim->setDiffuseColour(1.0f, 0.9f, 0.3f);
        rim->setSpotlightRange(Ogre::Degree(20.0f), Ogre::Degree(45.0f));

        mCamNode = mScene->getRootSceneNode()->createChildSceneNode("cam");
        mCamNode->setPosition(0.0f, 48.0f, 195.0f);
        mCam = mScene->createCamera("main");
        mCam->setNearClipDistance(0.1f);
        mCam->setFarClipDistance(2000.0f);
        mCam->setAutoAspectRatio(true);
        mCamNode->attachObject(mCam);
        mRenderWindow = getRenderWindow();
        mScene->setFog(
            Ogre::FOG_LINEAR, Ogre::ColourValue(0.04f, 0.05f, 0.1f), 0.001f, 80.0f, 360.0f);
        {
            Ogre::Viewport* const vp = mRenderWindow->addViewport(mCam);
            vp->setBackgroundColour(Ogre::ColourValue(0.04f, 0.05f, 0.1f));
            vp->setClearEveryFrame(true);
        }

        Ogre::MeshManager::getSingleton().createPlane(
            "assocGround", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::Plane(Ogre::Vector3::UNIT_Y, 0.0f), 2400.0f, 2400.0f, 32, 32, true, 1, 12.0f, 12.0f,
            Ogre::Vector3::UNIT_Z);

        Ogre::Entity* const ground = mScene->createEntity("assocGround");
        mGroundNode = mScene->getRootSceneNode()->createChildSceneNode("ground");
        mGroundNode->setPosition(0.0f, 0.0f, 0.0f);
        mGroundNode->attachObject(ground);
        ground->setMaterialName("Examples/Rockwall", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        mGroundNode->pitch(Ogre::Degree(180.0f));

        mHead = mScene->createEntity("assocHeadL", "ogrehead.mesh",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        mHeadNode = mScene->getRootSceneNode()->createChildSceneNode("headL",
            Ogre::Vector3(-72.0f, 18.0f, 15.0f));
        mHeadNode->setScale(0.6f, 0.6f, 0.6f);
        mHeadNode->yaw(Ogre::Degree(25.0f));
        mHeadNode->attachObject(mHead);

        mHead2 = mScene->createEntity("assocHeadR", "ogrehead.mesh",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        mHeadNode2 = mScene->getRootSceneNode()->createChildSceneNode("headR",
            Ogre::Vector3(72.0f, 18.0f, 12.0f));
        mHeadNode2->setScale(0.5f, 0.5f, 0.5f);
        mHeadNode2->yaw(Ogre::Degree(-30.0f));
        mHeadNode2->attachObject(mHead2);

        // --- 3D szöveg: `SdkTrays/Caption` + Essential. RTSS: `TVC_NONE`+diffúz gyakran fehér; helyette
        //     `TVC_DIFFUSE` + arany `colour` / csúcs. LINEAR min/mag → mosott: nearest + clamp.
        {
            Ogre::FontPtr const font{Ogre::FontManager::getSingleton().getByName(
                "SdkTrays/Caption", "Essential")};
            if (!font)
            {
                OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND, "Font SdkTrays/Caption (Essential) nem talalhato",
                    "AssocApp::setup");
            }
            // Ogre::Font: mMaterial csak loadImpl() után; getMaterial() / clone() előtt kötelező a load().
            font->load();
            Ogre::MaterialPtr m{font->getMaterial()};
            if (!m)
            {
                OGRE_EXCEPT(Ogre::Exception::ERR_INVALID_STATE, "Font anyag nincs (getMaterial) load utan", "AssocApp::setup");
            }
            m = m->clone("assoc/3DTextMat", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            Ogre::Technique* const te{m->getTechnique(0)};
            if (te)
            {
                Ogre::Pass* const pa{te->getPass(0)};
                if (pa)
                {
                    pa->setVertexColourTracking(Ogre::TVC_DIFFUSE);
                    pa->setDiffuse(1.0f, 1.0f, 1.0f, 1.0f);
                    pa->setFog(true, Ogre::FOG_NONE);
                    pa->setLightingEnabled(false);
                    pa->setSelfIllumination(0.0f, 0.0f, 0.0f);
                    pa->setDepthCheckEnabled(true);
                    if (pa->getNumTextureUnitStates() > 0)
                    {
                        Ogre::TextureUnitState* const tus{pa->getTextureUnitState(0U)};
                        tus->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);
                        tus->setTextureFiltering(
                            Ogre::FO_POINT, Ogre::FO_POINT, Ogre::FO_NONE);
                    }
                }
            }

            Ogre::uint32 id{0};
            mLineAssoc = mScene->getRootSceneNode()->createChildSceneNode("3dline_assoc", Ogre::Vector3(0.0F, 34.0F, 0.0F));
            add3DTextLine(mScene, mLineAssoc, "assoc3dL", *font, *m, "assoc", 16.0F, id);
            mLineAssoc->setAutoTracking(true, mCamNode, Ogre::Vector3::UNIT_Z, Ogre::Vector3::ZERO);

            mLineTop = mScene->getRootSceneNode()->createChildSceneNode("3dline_top", Ogre::Vector3(0.0F, 56.0F, 12.0F));
            add3DTextLine(mScene, mLineTop, "top3dL", *font, *m, "ASSOC  |  Ogre3D  |  3D", 4.2F, id);
            mLineTop->setAutoTracking(true, mCamNode, Ogre::Vector3::UNIT_Z, Ogre::Vector3::ZERO);

            mLineBottom = mScene->getRootSceneNode()->createChildSceneNode("3dline_bot", Ogre::Vector3(0.0F, 6.0F, 18.0F));
            add3DTextLine(
                mScene, mLineBottom, "bot3dL", *font, *m, "a s s o c  (nem csak 2D, 3D jelenet)", 3.2F, id);
            mLineBottom->setAutoTracking(true, mCamNode, Ogre::Vector3::UNIT_Z, Ogre::Vector3::ZERO);
        }
    }

    void shutdown() override
    {
        if (mScene)
        {
            if (mLineBottom)
            {
                mScene->destroySceneNode(mLineBottom);
                mLineBottom = nullptr;
            }
            if (mLineTop)
            {
                mScene->destroySceneNode(mLineTop);
                mLineTop = nullptr;
            }
            if (mLineAssoc)
            {
                mScene->destroySceneNode(mLineAssoc);
                mLineAssoc = nullptr;
            }
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
        if (mHeadNode)
        {
            mHeadNode->yaw(Ogre::Radian(evt.timeSinceLastFrame * -0.25f));
        }
        if (mHeadNode2)
        {
            mHeadNode2->yaw(Ogre::Radian(evt.timeSinceLastFrame * 0.2f));
        }
        if (mCamNode)
        {
            const float c = 8.0f * Ogre::Math::Cos(mT * 0.15f);
            const float s = 6.0f * Ogre::Math::Sin(mT * 0.12f);
            mCamNode->setPosition(c, 48.0f + s * 0.3f, 195.0f);
            mCamNode->lookAt(Ogre::Vector3(0.0f, 24.0f, 0.0f), Ogre::Node::TS_WORLD, Ogre::Vector3::NEGATIVE_UNIT_Z);
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

    Ogre::SceneNode* mHeadNode{nullptr};
    Ogre::Entity* mHead{nullptr};
    Ogre::SceneNode* mHeadNode2{nullptr};
    Ogre::Entity* mHead2{nullptr};

    Ogre::SceneNode* mLineAssoc{nullptr};
    Ogre::SceneNode* mLineTop{nullptr};
    Ogre::SceneNode* mLineBottom{nullptr};

    Ogre::Real mT{0.0f};
};

int main(int /*argc*/, char* /*argv*/[])
{
    try
    {
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
