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

#include <cmath>
#include <cstdint>
#include <memory>
#include <vector>

namespace
{
    Ogre::MaterialPtr makeLitMaterial(Ogre::String const& name, Ogre::ColourValue const& diffuse,
        Ogre::ColourValue const& specular = Ogre::ColourValue(0.12f, 0.1f, 0.07f, 1.0f),
        Ogre::Real shininess = 12.0f)
    {
        Ogre::MaterialPtr material{Ogre::MaterialManager::getSingleton().create(
            name, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME)};
        Ogre::Pass* const pass{material->getTechnique(0)->getPass(0)};
        pass->setLightingEnabled(true);
        pass->setDiffuse(diffuse);
        pass->setAmbient(diffuse * 0.42f);
        pass->setSpecular(specular);
        pass->setShininess(shininess);
        return material;
    }

    void addQuad(Ogre::ManualObject* const mo, Ogre::Vector3 const& a, Ogre::Vector3 const& b,
        Ogre::Vector3 const& c, Ogre::Vector3 const& d, Ogre::Vector3 const& normal)
    {
        mo->position(a);
        mo->normal(normal);
        mo->position(b);
        mo->normal(normal);
        mo->position(c);
        mo->normal(normal);
        mo->position(a);
        mo->normal(normal);
        mo->position(c);
        mo->normal(normal);
        mo->position(d);
        mo->normal(normal);
    }

    void addBox(Ogre::SceneManager* const scene, Ogre::SceneNode* const parent, Ogre::String const& name,
        Ogre::Vector3 const& centre, Ogre::Vector3 const& size, Ogre::Material const& material)
    {
        Ogre::Vector3 const h{size * 0.5f};
        Ogre::Vector3 const v[8] = {
            centre + Ogre::Vector3(-h.x, -h.y, -h.z), centre + Ogre::Vector3(h.x, -h.y, -h.z),
            centre + Ogre::Vector3(h.x, h.y, -h.z), centre + Ogre::Vector3(-h.x, h.y, -h.z),
            centre + Ogre::Vector3(-h.x, -h.y, h.z), centre + Ogre::Vector3(h.x, -h.y, h.z),
            centre + Ogre::Vector3(h.x, h.y, h.z), centre + Ogre::Vector3(-h.x, h.y, h.z)};

        Ogre::ManualObject* const mo{scene->createManualObject(name)};
        mo->begin(material.getName(), Ogre::RenderOperation::OT_TRIANGLE_LIST);
        addQuad(mo, v[4], v[5], v[6], v[7], Ogre::Vector3::UNIT_Z);
        addQuad(mo, v[1], v[0], v[3], v[2], Ogre::Vector3::NEGATIVE_UNIT_Z);
        addQuad(mo, v[0], v[4], v[7], v[3], Ogre::Vector3::NEGATIVE_UNIT_X);
        addQuad(mo, v[5], v[1], v[2], v[6], Ogre::Vector3::UNIT_X);
        addQuad(mo, v[3], v[7], v[6], v[2], Ogre::Vector3::UNIT_Y);
        addQuad(mo, v[0], v[1], v[5], v[4], Ogre::Vector3::NEGATIVE_UNIT_Y);
        mo->end();

        Ogre::SceneNode* const node{parent->createChildSceneNode(name + "_node")};
        node->attachObject(mo);
    }

    void addHalfBamboo(Ogre::SceneManager* const scene, Ogre::SceneNode* const parent, Ogre::String const& name,
        Ogre::Vector3 const& centre, Ogre::Real const length, Ogre::Real const radius, Ogre::Degree const angle,
        Ogre::Material const& material)
    {
        Ogre::ManualObject* const mo{scene->createManualObject(name)};
        mo->begin(material.getName(), Ogre::RenderOperation::OT_TRIANGLE_LIST);

        constexpr int segments{14};
        Ogre::Real const halfLength{length * 0.5f};
        for (int i = 0; i < segments; ++i)
        {
            Ogre::Real const t0{Ogre::Math::PI * static_cast<Ogre::Real>(i) / segments};
            Ogre::Real const t1{Ogre::Math::PI * static_cast<Ogre::Real>(i + 1) / segments};
            Ogre::Vector3 const p0{-radius * std::cos(t0), -halfLength, radius * std::sin(t0)};
            Ogre::Vector3 const p1{-radius * std::cos(t1), -halfLength, radius * std::sin(t1)};
            Ogre::Vector3 const p2{-radius * std::cos(t1), halfLength, radius * std::sin(t1)};
            Ogre::Vector3 const p3{-radius * std::cos(t0), halfLength, radius * std::sin(t0)};
            Ogre::Vector3 const n0{-std::cos(t0), 0.0f, std::sin(t0)};
            Ogre::Vector3 const n1{-std::cos(t1), 0.0f, std::sin(t1)};

            mo->position(p0);
            mo->normal(n0.normalisedCopy());
            mo->position(p1);
            mo->normal(n1.normalisedCopy());
            mo->position(p2);
            mo->normal(n1.normalisedCopy());
            mo->position(p0);
            mo->normal(n0.normalisedCopy());
            mo->position(p2);
            mo->normal(n1.normalisedCopy());
            mo->position(p3);
            mo->normal(n0.normalisedCopy());
        }
        addQuad(mo, Ogre::Vector3(radius, -halfLength, 0.0f), Ogre::Vector3(-radius, -halfLength, 0.0f),
            Ogre::Vector3(-radius, halfLength, 0.0f), Ogre::Vector3(radius, halfLength, 0.0f),
            Ogre::Vector3::NEGATIVE_UNIT_Z);
        mo->end();

        Ogre::SceneNode* const node{parent->createChildSceneNode(name + "_node", centre)};
        node->roll(angle);
        node->attachObject(mo);
    }

    void addBambooJoint(Ogre::SceneManager* const scene, Ogre::SceneNode* const parent, Ogre::String const& name,
        Ogre::Vector3 const& centre, Ogre::Real const width, Ogre::Real const radius, Ogre::Degree const angle,
        Ogre::Material const& material)
    {
        addHalfBamboo(scene, parent, name, centre, width, radius * 1.08f, angle, material);
    }

    void addDiagonalBambooWall(Ogre::SceneManager* const scene, Ogre::SceneNode* const parent,
        Ogre::Material const& woodMat, Ogre::Material const& darkWoodMat, Ogre::Material const& bambooMat,
        Ogre::Material const& bambooAltMat, Ogre::Material const& jointMat)
    {
        Ogre::SceneNode* const wall{parent->createChildSceneNode("bamboo_wall", Ogre::Vector3(0.0f, 34.0f, 0.0f))};

        addBox(scene, wall, "left_post", Ogre::Vector3(-112.0f, 0.0f, 0.0f), Ogre::Vector3(11.0f, 82.0f, 11.0f), woodMat);
        addBox(scene, wall, "right_post", Ogre::Vector3(112.0f, 0.0f, 0.0f), Ogre::Vector3(11.0f, 82.0f, 11.0f), woodMat);
        addBox(scene, wall, "left_post_shadow", Ogre::Vector3(-108.7f, 0.0f, 5.9f), Ogre::Vector3(2.0f, 76.0f, 1.8f), darkWoodMat);
        addBox(scene, wall, "right_post_shadow", Ogre::Vector3(115.3f, 0.0f, 5.9f), Ogre::Vector3(2.0f, 76.0f, 1.8f), darkWoodMat);
        addBox(scene, wall, "top_rail", Ogre::Vector3(0.0f, 36.0f, -1.0f), Ogre::Vector3(212.0f, 5.0f, 7.0f), darkWoodMat);
        addBox(scene, wall, "bottom_rail", Ogre::Vector3(0.0f, -36.0f, -1.0f), Ogre::Vector3(212.0f, 5.0f, 7.0f), darkWoodMat);
        addBox(scene, wall, "left_foot", Ogre::Vector3(-112.0f, -45.0f, 0.0f), Ogre::Vector3(22.0f, 6.0f, 15.0f), darkWoodMat);
        addBox(scene, wall, "right_foot", Ogre::Vector3(112.0f, -45.0f, 0.0f), Ogre::Vector3(22.0f, 6.0f, 15.0f), darkWoodMat);

        constexpr Ogre::Real length{106.0f};
        constexpr Ogre::Real radius{3.0f};
        int id{0};
        for (Ogre::Real x = -72.0f; x <= 72.0f; x += 24.0f)
        {
            Ogre::Material const& mat{(id % 2 == 0) ? bambooMat : bambooAltMat};
            addHalfBamboo(scene, wall, "bamboo_diag_a_" + Ogre::StringConverter::toString(id),
                Ogre::Vector3(x, 0.0f, 4.8f), length, radius, Ogre::Degree(-25.0f), mat);
            addHalfBamboo(scene, wall, "bamboo_diag_b_" + Ogre::StringConverter::toString(id),
                Ogre::Vector3(x, 0.0f, 8.8f), length, radius, Ogre::Degree(25.0f), mat);
            for (Ogre::Real offset : {-28.0f, 0.0f, 28.0f})
            {
                addBambooJoint(scene, wall, "joint_a_" + Ogre::StringConverter::toString(id) + "_" +
                        Ogre::StringConverter::toString(static_cast<int>(offset + 30.0f)),
                    Ogre::Vector3(x + std::sin(Ogre::Degree(-25.0f).valueRadians()) * offset,
                        std::cos(Ogre::Degree(-25.0f).valueRadians()) * offset, 5.4f),
                    3.0f, radius, Ogre::Degree(-25.0f), jointMat);
                addBambooJoint(scene, wall, "joint_b_" + Ogre::StringConverter::toString(id) + "_" +
                        Ogre::StringConverter::toString(static_cast<int>(offset + 30.0f)),
                    Ogre::Vector3(x + std::sin(Ogre::Degree(25.0f).valueRadians()) * offset,
                        std::cos(Ogre::Degree(25.0f).valueRadians()) * offset, 9.4f),
                    3.0f, radius, Ogre::Degree(25.0f), jointMat);
            }
            ++id;
        }
    }

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
        mCamNode->setPosition(0.0f, 48.0f, 270.0f);
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
            Ogre::Vector3(-145.0f, 16.0f, 8.0f));
        mHeadNode->setScale(0.42f, 0.42f, 0.42f);
        mHeadNode->yaw(Ogre::Degree(25.0f));
        mHeadNode->attachObject(mHead);

        mHead2 = mScene->createEntity("assocHeadR", "ogrehead.mesh",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
        mHeadNode2 = mScene->getRootSceneNode()->createChildSceneNode("headR",
            Ogre::Vector3(145.0f, 16.0f, 8.0f));
        mHeadNode2->setScale(0.38f, 0.38f, 0.38f);
        mHeadNode2->yaw(Ogre::Degree(-30.0f));
        mHeadNode2->attachObject(mHead2);

        Ogre::MaterialPtr const woodMat{makeLitMaterial("assoc/warmWood", Ogre::ColourValue(0.47f, 0.28f, 0.11f))};
        Ogre::MaterialPtr const darkWoodMat{
            makeLitMaterial("assoc/darkWood", Ogre::ColourValue(0.19f, 0.11f, 0.045f))};
        Ogre::MaterialPtr const bambooMat{
            makeLitMaterial("assoc/goldBamboo", Ogre::ColourValue(0.86f, 0.63f, 0.18f), Ogre::ColourValue(0.55f, 0.42f, 0.16f), 24.0f)};
        Ogre::MaterialPtr const bambooAltMat{
            makeLitMaterial("assoc/greenBamboo", Ogre::ColourValue(0.42f, 0.58f, 0.24f), Ogre::ColourValue(0.26f, 0.36f, 0.18f), 18.0f)};
        Ogre::MaterialPtr const jointMat{
            makeLitMaterial("assoc/bambooJoints", Ogre::ColourValue(0.98f, 0.82f, 0.22f), Ogre::ColourValue(0.75f, 0.55f, 0.18f), 30.0f)};
        mWallNode = mScene->getRootSceneNode()->createChildSceneNode("wallRoot");
        addDiagonalBambooWall(mScene, mWallNode, *woodMat, *darkWoodMat, *bambooMat, *bambooAltMat, *jointMat);

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
            mLineAssoc = mScene->getRootSceneNode()->createChildSceneNode("3dline_assoc", Ogre::Vector3(0.0F, 88.0F, 10.0F));
            add3DTextLine(mScene, mLineAssoc, "assoc3dL", *font, *m, "bambusz", 8.4F, id);
            mLineAssoc->setAutoTracking(true, mCamNode, Ogre::Vector3::UNIT_Z, Ogre::Vector3::ZERO);

            mLineTop = mScene->getRootSceneNode()->createChildSceneNode("3dline_top", Ogre::Vector3(0.0F, 76.0F, 14.0F));
            add3DTextLine(mScene, mLineTop, "top3dL", *font, *m, "ATLOS  |  OGRE3D  |  VALASZTOFAL", 3.8F, id);
            mLineTop->setAutoTracking(true, mCamNode, Ogre::Vector3::UNIT_Z, Ogre::Vector3::ZERO);

            mLineBottom = mScene->getRootSceneNode()->createChildSceneNode("3dline_bot", Ogre::Vector3(0.0F, -20.0F, 18.0F));
            add3DTextLine(
                mScene, mLineBottom, "bot3dL", *font, *m, "felhengerek ket tavoli fa oszlop kozott", 3.1F, id);
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
        if (mWallNode)
        {
            mWallNode->setPosition(0.0f, 0.0f, 2.0f * Ogre::Math::Sin(mT * 0.55f));
            mWallNode->setOrientation(
                Ogre::Quaternion(Ogre::Degree(1.6f * Ogre::Math::Sin(mT * 0.32f)), Ogre::Vector3::UNIT_Y));
        }
        if (mCamNode)
        {
            const float c = 10.0f * Ogre::Math::Cos(mT * 0.15f);
            const float s = 6.0f * Ogre::Math::Sin(mT * 0.12f);
            mCamNode->setPosition(c, 48.0f + s * 0.3f, 270.0f);
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

    Ogre::SceneNode* mWallNode{nullptr};

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
