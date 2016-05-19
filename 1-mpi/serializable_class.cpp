#include <cereal/access.hpp>

class ISerializableClass {
public:
    friend class cereal::access;

    template <class Archive>
    void serialize( Archive & ar )
    {
      ar(  );
    }
};