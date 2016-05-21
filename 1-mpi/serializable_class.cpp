#include <cereal/access.hpp>

class ISerializableClass {
public:
	long dataTmstmp;
    friend class cereal::access;

    template <class Archive>
    void serialize( Archive & ar )
    {
      ar( dataTmstmp );
    }
};