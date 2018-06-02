# Halo Online Wrapper

## Entity Concepts

As we're not creating the entities ourself, but just wrapping around Halo's,
which is component based, it would be a pain to botch some polymorphism together
to correctly handle it. For this reason, I propose the following, which is a sort of
duck-typing (tho type safe) extension system which can extend a basic entity handle:

### Base Entity

Each entity has a unique ID, along with a handle for ElDorito, and then various methods that can be attached.

```c++
class Entity
{
private:
	int Uid; // not sure if BlamHandle is 100% unique
	Blam::DatumHandle BlamHandle;
	Blam::Objects::ObjectBase* BlamObject(); // this can't be cached i think

public:
	static const std::vector<std::shared_ptr<Entity>>& GetAll();
	template<typename T>
	static const std::vector<std::shared_ptr<Entity>>& GetAll();
};
```

### Entity Extensions

Allow attaching arbitrary data onto an entity.

```c++
class Entity
{
public:
	template<typename T>
	T& Extension();

	template<typename T>
	bool HasExtension<T>();
};
```


`T` must have a `T(const std::shared_ptr<Entity>& parent)` constructor,
which will construct a default extension for this entity
and, if needed, save a copy of `parent` using an `std::weak_ref<Entity>`.

Extensions will remain attached until the entity destructor is called,
tho I might add `Entity::ClearAllExtensions<T>()` if desired.

#### Example

```c++
std::shared_ptr<Entity> pl = Entities::GetAll<Player>()[0];

Friend& ext = pl->Extension<Friend>();
ext.IsFriend = pl->Extension<Player>().Name == "xyz";
```

#### Possible Extensions

##### Core Game

 - Translation
    * Position
    * Rotation
    * Scale
    * Derivative calculator
      + Velocity
      + Acceleration
      + Jerk
 - Player
    * Name
    * Team
    * Current weapon
 - Vehicle
 - Weapon
    * Barrels
    * Fire speed
    * Projectile speed
 - Ammo

##### Hack

 - Friend (possibly built into the Score extension)
 - Translation (mixed with core for derivative tracking)
 - Score (last calculated score and any introspective values for it)