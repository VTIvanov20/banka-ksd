#include "./Component.hpp"
#include "../Entity.hpp"

void Component::BindToEntity(std::weak_ptr<Object> object)
{
    /**
     * @brief Binds the component to an entity
     */
    assert(!object.expired() && "Passed an expired pointer");
    assert(ObjectManager::CheckBaseName(object, "Entity") && "The passed object was not an Entity");
    IsUsed = true;
    boundEntity = object;
}

void Component::BindToEntity(unsigned long long _id)
{
    /**
     * @brief Gets the entity by id and binds the component to it
     */
    auto object = ObjectManager::GetInstance()->GetObjectFromID(_id);
    BindToEntity(object);
}

std::weak_ptr<Component> Component::GetComponent(std::string componentName)
{
    /**
     * @brief Gets the current entity and returns the component with the desired name
     */
    auto entity = std::static_pointer_cast<Entity>(boundEntity.lock());
    return entity->GetComponent(componentName);
}