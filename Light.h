/*
* This file is part of the ungod - framework.
* Copyright (C) 2016 Felix Becker - fb132550@uni-greifswald.de
*
* This is a modified version of the Let There Be Light 2 framework.
* See https://github.com/222464/LTBL2
*
* This software is provided 'as-is', without any express or
* implied warranty. In no event will the authors be held
* liable for any damages arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute
* it freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented;
*    you must not claim that you wrote the original software.
*    If you use this software in a product, an acknowledgment
*    in the product documentation would be appreciated but
*    is not required.
*
* 2. Altered source versions must be plainly marked as such,
*    and must not be misrepresented as being the original software.
*
* 3. This notice may not be removed or altered from any
*    source distribution.
*/

#ifndef LIGHT_H
#define LIGHT_H

#include <SFML/Graphics.hpp>
#include "owls/Signal.h"
#include "quadtree/QuadTree.h"
#include "ungod/visual/Image.h"
#include "ungod/base/Transform.h"

namespace ungod
{
    struct Penumbra;

    /** \brief A base class for lights and light-colliders. Provides basic functionality for
    * disable and enable the derived device. */
    class BaseLight
    {
    public:
        BaseLight();

        /** \brief Sets the active status. */
        void setActive(bool active);

        /** \brief Returns true if the device is currently active. */
        bool isActive() const;

        /** \brief Toggles the active status (flips the bool). */
        void toggleActive();

        /** \brief Renders penumbras to the texture. */
        void unmaskWithPenumbras(sf::RenderTexture& renderTexture,
                                 sf::RenderStates states,
                                 sf::Shader& unshadowShader,
                                 const std::vector<Penumbra>& penumbras,
                                 float shadowExtension) const;

    private:
        bool mActive; ///<states whether the object is currently active, that means it performs its underlying actions
    };

    /** \brief A collider for lights. Will cause the casting of shadows.
    * Light colliders are assumed to be convex polygons. */
    class LightCollider : public BaseLight
    {
    friend class LightSystem;
    public:
        LightCollider();
        LightCollider(std::size_t numPoints);

        sf::FloatRect getBoundingBox() const;

        void setPointCount(std::size_t numPoints);

        std::size_t getPointCount() const;

        void setPoint(std::size_t index, const sf::Vector2f& point);

        sf::Vector2f getPoint(std::size_t index) const;

        const sf::Transform& getTransform() const;

        bool getLightOverShape() const;

        void setLightOverShape(bool los);

        void render(sf::RenderTarget& target, sf::RenderStates states);

        void setColor(const sf::Color& color);

    private:
        sf::ConvexShape mShape;
        bool mLightOverShape;
    };


    /** \brief A lightsource that emits light from a source point for certain radius.
    * The rendered light will break on LightColliders withins the lights radius and will cast
    * shadows with natural penumbras/antumbra. */
    class PointLight : public BaseLight
    {
    friend class LightSystem;
    friend class LightFlickering;
    friend class RandomizedFlickering;
    public:
        PointLight(const std::string& texturePath = DEFAULT_TEXTURE_PATH);

        sf::FloatRect getBoundingBox() const;

        void render(const sf::View& view,
                    sf::RenderTexture& lightTexture,
                    sf::RenderTexture& emissionTexture,
                    sf::RenderTexture& antumbraTexture,
                    const std::vector< std::pair<LightCollider*, Transform*> >& colliders,
                    sf::Shader& unshadowShader,
                    sf::Shader& lightOverShapeShader,
                    const Transform& transf) const;

        /** \brief Loads a texture for the light source. Replaces the default texture. */
        void loadTexture(const std::string& path = DEFAULT_TEXTURE_PATH);

        /** \brief Returns the current color of the light. */
        sf::Color getColor() const;

        /** \brief Sets the color of the light. */
        void setColor(const sf::Color& color);

        /** \brief Sets the size of the light. */
        sf::Vector2f getScale() const;

        /** \brief Gets the current local position of the light. */
        sf::Vector2f getPosition() const;

        /** \brief Sets the source point (where the lights origin is) in local coordinates. */
        void setSourcePoint(const sf::Vector2f& source);

        /** \brief Returns the source point (where the lights origin is) in local coordinates. */
        sf::Vector2f getSourcePoint() const;

        /** \brief Returns the correctly transformed source point of the light.
        * This is the center position of the underlying sprite with all
        * transformations applied. */
        sf::Vector2f getCastCenter() const;

        /** \brief Computes the coordinates of the penumbras in 2d space. */
        void getPenumbrasPoint(std::vector<Penumbra>& penumbras,
                               std::vector<int>& innerBoundaryIndices,
                               std::vector<sf::Vector2f>& innerBoundaryVectors,
                               std::vector<int>& outerBoundaryIndices,
                               std::vector<sf::Vector2f>& outerBoundaryVectors,
                               const LightCollider& collider,
                               const Transform& colliderTransform,
                               const Transform& lightTransform) const;

    private:
        sf::Sprite mSprite;
        sf::Vector2f mSourcePoint;
        float mRadius;
        float mShadowOverExtendMultiplier;
        Image mTexture;

        static const std::string DEFAULT_TEXTURE_PATH;
    };

    /** \brief A struct modelling a penumbra (border reagion of a shadow). */
    struct Penumbra
    {
        sf::Vector2f source;
        sf::Vector2f lightEdge;
        sf::Vector2f darkEdge;
        float lightBrightness;
        float darkBrightness;
        float distance;
    };


    /** \brief A component for entities that should have the ability to block light and
    * cast shadows. */
    class ShadowEmitter
    {
    friend class LightSystem;
    private:
        LightCollider mLightCollider;
    };

    /** \brief Same as shadow emitter but can hold multiple colliders (for a small overhead). Use only if collider
    * count will be greater then 1. */
    using MultiShadowEmitter = dom::MultiComponent<ShadowEmitter>;


    /** \brief A component for entities that should have the ability to hold exactly one point light. */
    class LightEmitter
    {
    friend class LightSystem;
    friend class LightFlickering;
    friend class RandomizedFlickering;
    private:
        PointLight mLight;
    public:
        PointLight& getLight() { return mLight; }
        const PointLight& getLight() const { return mLight; }
    };

    /** \brief A component for entities that should have the ability to hold multiple point lights. */
    using MultiLightEmitter = dom::MultiComponent<LightEmitter>;


    /** \brief A component that can be attached to an entity in addition to a LightEmitter. This component
    * will apply effects to the light that are defined in a callback function, that are updated each frame.
    * For example flickering. This file also provides templates for those callback functions. */
    class LightAffector
    {
    friend class LightSystem;
    private:
        std::function<void(float, LightEmitter&)> mCallback;
        LightEmitter* mEmitter;
        bool mActive;

    public:
        LightAffector();

        void setActive(bool active);

        bool isActive() const;
    };

    /** \brief A LightAffector that can affect multiple lights. Only meaningful, if used in combination with a
    * MultiLightEmitter-component. */
    using MultiLightAffector = dom::MultiComponent<LightAffector>;


    /** \brief A callable object, that can be used as a callback for LightAffector.
    * LightFlickering will make a light object flicker, randomly or continously.
    * Note that you can also use this template in your own callbacks.
    * Also note that this light will modify the scale of the lights-sprite. However the bounding rect of the light
    * is promised to be never bigger then in the original state and the quad-tree doesnt have to be updated when flickering. */
    class LightFlickering
    {
    private:
        bool mDirection;
        const float mPeriod;
        const float mStrength;
        sf::Clock mTimer;

    public:
        LightFlickering(float period, float strength);

        void operator() (float delta, LightEmitter& light);
    };
    class RandomizedFlickering
    {
    private:
        bool mDirection;
        float mPeriod;
        const float mBasePeriod;
        const float mStrength;
        sf::Clock mTimer;
        float mSizeMemorizer;

    public:
        RandomizedFlickering(float basePeriod, float strength);

        void operator() (float delta, LightEmitter& light);
    };


    /** \brief The bring-it-all-together class. Handles all Lights and LightColliders and is responsible for
    * rendering them. */
    class LightSystem : sf::NonCopyable
    {
    public:
        LightSystem();

        /** \brief Instantiates the light system will a pointer to the world-quadtree and filepaths to the required shaders.
        * Also requires a path to the penumbra-texture. */
        void init(quad::QuadTree<Entity>* quadtree,
                  const sf::Vector2u &imageSize,
                  const std::string& unshadowVertex,
                  const std::string& unshadowFragment,
                  const std::string& lightOverShapeVertex,
                  const std::string& lightOverShapeFragment,
                  const std::string& penumbraTexture);

        /** \brief Updates the size of the underlying render-textures (e.g. if the window was resized). */
        void setImageSize(const sf::Vector2u &imageSize);

        /** \brief Renders lights and lightcolliders of a list of entities. */
        void render(const quad::PullResult<Entity>& pull, sf::RenderTarget& target, sf::RenderStates states);

        /** \brief Updates LightAffectors. */
        void update(const std::list<Entity>& entities, float delta);

        /** \brief Sets the color of the ambient light. */
        void setAmbientColor(const sf::Color& color);

        /**
        * \brief Calling this function over a certain amount of time will result in
        * smoothly color transform to the given color. The strength value should somehow
        * depend on the applications delta value.
        */
        void interpolateAmbientLight(const sf::Color& color, float strength);

        /** \brief Sets the local position of the light of entity e if a LightEmitter
        * component is attached. */
        void setLocalLightPosition(Entity e, const sf::Vector2f& position);

        /** \brief Sets the local position of the light of entity e if a MultiLightEmitter
        * component is attached. */
        void setLocalLightPosition(Entity e, const sf::Vector2f& position, std::size_t index);

        /** \brief Sets the scale of the light of entity e. Requires a LightEmitter component. */
        void setLightScale(Entity e, const sf::Vector2f& scale);

        /** \brief Sets the scale of the light with given index of entity e. Requires a MultiLightEmitter component. */
        void setLightScale(Entity e, const sf::Vector2f& scale, std::size_t index);

        /** \brief Sets the color of the light of entity e. Requires a LightEmitter component. */
        void setLightColor(Entity e, const sf::Color& color);

        /** \brief Sets the color of the light with given index of entity e. Requires a MultiLightEmitter component. */
        void setLightColor(Entity e, const sf::Color& color, std::size_t index);

        /** \brief Sets the coordinates of the ith point of the LightCollider. Requires ShadowEmitter component. */
        void setPoint(Entity e, const sf::Vector2f& point, std::size_t i);

        /** \brief Sets the coordinates of the ith point of the LightCollider with given index. Requires MultiShadowEmitter component. */
        void setPoint(Entity e, const sf::Vector2f& point, std::size_t pointIndex, std::size_t colliderIndex);

        /** \brief Sets the whole coordinate-set of the of the LightCollider. Requires ShadowEmitter component. */
        void setPoints(Entity e, const std::vector<sf::Vector2f>& points);

        /** \brief Sets the whole coordinate-set of the of the LightCollider with given index. Requires MultiShadowEmitter component. */
        void setPoints(Entity e, const std::vector<sf::Vector2f>& points, std::size_t colliderIndex);


        /** \brief Defines the callback for the affector. Is mandatory to get the
        * affector to work. Requires LightEmitter-component and a LightEffector-component. */
        void setAffectorCallback(Entity e, const std::function<void(float, LightEmitter&)>& callback);

        /** \brief Sets the affector for a MultiLightEmitter-component. */
        void setAffectorCallback(Entity e, std::size_t lightIndex, const std::function<void(float, LightEmitter&)>& callback);

        /** \brief Sets the affector for a MultiLightEmitter-component and MultiLightEmitter-component. */
        void setAffectorCallback(Entity e, std::size_t lightIndex, std::size_t affectorIndex, const std::function<void(float, LightEmitter&)>& callback);


        /** \brief Registers new callback for the ContentsChanged signal. */
        void onContentsChanged(const std::function<void(Entity, const sf::IntRect&)>& callback);

        /** \brief Methods that move all lights/colliders attached to the given entity. Are usually only
        * used internally by the transform-manager. */
        void moveLights(Entity e, const sf::Vector2f& vec);
        void moveLightColliders(Entity e, const sf::Vector2f& vec);


    private:
        sf::RenderTexture mLightTexture, mEmissionTexture, mAntumbraTexture, mCompositionTexture;
        sf::Shader mUnshadowShader, mLightOverShapeShader;
        quad::QuadTree<Entity>* mQuadTree;
        ungod::Image mPenumbraTexture;
        sf::Color mAmbientColor;
        sf::Sprite mDisplaySprite;
        sf::Vector3f mColorShift;
        owls::Signal<Entity, const sf::IntRect&> mContentsChangedSignal;

    private:
        void setAffectorCallback(const std::function<void(float, LightEmitter&)>& callback, LightAffector& affector, LightEmitter& emitter);
        void renderLight(sf::RenderTarget& target, sf::RenderStates states, Entity e, Transform& lightTransf, LightEmitter& light);
    };


    //define serialization behavior
    template <>
    struct SerialIdentifier<ShadowEmitter>
    {
        static std::string get()  { return "ShadowEmitter"; }
    };
    template <>
    struct SerialBehavior<ShadowEmitter>
    {
        static void serialize(const ShadowEmitter& data, MetaNode serializer, SerializationContext& context);
    };
    template <>
    struct DeserialBehavior<ShadowEmitter>
    {
        static void deserialize(ShadowEmitter& data, MetaNode deserializer, DeserializationContext& context);
    };

    template <>
    struct SerialIdentifier<MultiShadowEmitter>
    {
        static std::string get()  { return "MultiShadowEmitter"; }
    };
    template <>
    struct SerialBehavior<MultiShadowEmitter>
    {
        static void serialize(const MultiShadowEmitter& data, MetaNode serializer, SerializationContext& context);
    };
    template <>
    struct DeserialBehavior<MultiShadowEmitter>
    {
        static void deserialize(MultiShadowEmitter& data, MetaNode deserializer, DeserializationContext& context);
    };

    template <>
    struct SerialIdentifier<LightEmitter>
    {
        static std::string get()  { return "LightEmitter"; }
    };
    template <>
    struct SerialBehavior<LightEmitter>
    {
        static void serialize(const LightEmitter& data, MetaNode serializer, SerializationContext& context);
    };
    template <>
    struct DeserialBehavior<LightEmitter>
    {
        static void deserialize(LightEmitter& data, MetaNode deserializer, DeserializationContext& context);
    };

    template <>
    struct SerialIdentifier<MultiLightEmitter>
    {
        static std::string get()  { return "MultiLightEmitter"; }
    };
    template <>
    struct SerialBehavior<MultiLightEmitter>
    {
        static void serialize(const MultiLightEmitter& data, MetaNode serializer, SerializationContext& context);
    };
    template <>
    struct DeserialBehavior<MultiLightEmitter>
    {
        static void deserialize(MultiLightEmitter& data, MetaNode deserializer, DeserializationContext& context);
    };

    template <>
    struct SerialIdentifier<LightAffector>
    {
        static std::string get()  { return "LightAffector"; }
    };
    template <>
    struct SerialBehavior<LightAffector>
    {
        static void serialize(const LightAffector& data, MetaNode serializer, SerializationContext& context);
    };
    template <>
    struct DeserialBehavior<LightAffector>
    {
        static void deserialize(LightAffector& data, MetaNode deserializer, DeserializationContext& context);
    };

    template <>
    struct SerialIdentifier<MultiLightAffector>
    {
        static std::string get()  { return "MultiLightAffector"; }
    };
    template <>
    struct SerialBehavior<MultiLightAffector>
    {
        static void serialize(const MultiLightAffector& data, MetaNode serializer, SerializationContext& context);
    };
    template <>
    struct DeserialBehavior<MultiLightAffector>
    {
        static void deserialize(MultiLightAffector& data, MetaNode deserializer, DeserializationContext& context);
    };
}

#endif //LIGHT_H
