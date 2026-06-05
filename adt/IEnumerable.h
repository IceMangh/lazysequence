#ifndef IENUMERABLE_H
#define IENUMERABLE_H

#include "IEnumerator.h"

template <class T>
class IEnumerable {
public:
    virtual IEnumerator<T>* GetEnumerator() const = 0;
    virtual ~IEnumerable() = default;
};

#endif