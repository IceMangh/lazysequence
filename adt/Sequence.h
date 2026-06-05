#pragma once

#include "IEnumerable.h"
#include "ICollection.h"

template <class T>
class Sequence : public IEnumerable<T>, public ICollection<T> {
protected:
    void AppendToResult(Sequence<T>*& result, const T& item) const {
        Sequence<T>* current = result;
        Sequence<T>* updated = current->Append(item);

        if (updated != current) {
            delete current;
            result = updated;
        }
    }

public:
    virtual ~Sequence() = default;

    virtual const T& GetFirst() const = 0;
    virtual const T& GetLast() const = 0;
    virtual const T& Get(int index) const override = 0;
    virtual int GetLength() const override = 0;

    virtual Sequence<T>* GetSubsequence(int startIndex, int endIndex) const = 0;
    virtual Sequence<T>* Append(const T& item) = 0;
    virtual Sequence<T>* Prepend(const T& item) = 0;
    virtual Sequence<T>* InsertAt(const T& item, int index) = 0;
    virtual Sequence<T>* Concat(const Sequence<T>& other) const = 0;

    virtual Sequence<T>* Clone() const override = 0;
    virtual Sequence<T>* CreateEmpty() const = 0;

    template <class Mapper>
    Sequence<T>* Map(Mapper mapper) const {
        Sequence<T>* result = CreateEmpty();
        IEnumerator<T>* enumerator = this->GetEnumerator();

        try {
            while (enumerator->MoveNext()) {
                AppendToResult(result, mapper(enumerator->Current()));
            }
            delete enumerator;
            return result;
        } catch (...) {
            delete enumerator;
            delete result;
            throw;
        }
    }

    template <class Reducer>
    T Reduce(Reducer reducer, T start) const {
        T result = start;
        IEnumerator<T>* enumerator = this->GetEnumerator();

        try {
            while (enumerator->MoveNext()) {
                result = reducer(result, enumerator->Current());
            }
            delete enumerator;
            return result;
        } catch (...) {
            delete enumerator;
            throw;
        }
    }

    bool operator==(const Sequence<T>& other) const {
        if (GetLength() != other.GetLength()) {
            return false;
        }

        for (int i = 0; i < GetLength(); ++i) {
            if (Get(i) != other.Get(i)) {
                return false;
            }
        }

        return true;
    }

    bool operator!=(const Sequence<T>& other) const {
        return !(*this == other);
    }

    Sequence<T>* operator+(const Sequence<T>& other) const {
        return Concat(other);
    }
};
