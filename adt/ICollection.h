#ifndef ICOLLECTION_H
#define ICOLLECTION_H

template <class T>
class ICollection {
public:
    virtual const T& Get(int index) const = 0;
    virtual int GetLength() const = 0;
    virtual ICollection<T>* Clone() const = 0;
    virtual ~ICollection() = default;
};

#endif
