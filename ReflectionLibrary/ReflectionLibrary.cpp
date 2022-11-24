#include <iostream>
#include <vector>

using cstr_t = const char *;
using unk_t = void *;

#define PUBLIC_CSTR      "public"
#define PROTECTED_CSTR   "protected"
#define PRIVATE_CSTR     "private"

enum class EFieldAccess
{
    UNKNOWN = 0,
    PUBLIC,
    PROTECTED,
    PRIVATE,
};

EFieldAccess ParseAccess(cstr_t accessStr)
{
    if (!strcmp(accessStr, PUBLIC_CSTR))
        return EFieldAccess::PUBLIC;

    if (!strcmp(accessStr, PROTECTED_CSTR))
        return EFieldAccess::PROTECTED;

    if (!strcmp(accessStr, PRIVATE_CSTR))
        return EFieldAccess::PRIVATE;

    return EFieldAccess::UNKNOWN;
}

enum class EPrimitiveId
{
    UNKNOWN = 0,
    VOID,
    BOOL,
    CHAR,
    INT,
    SHORT,
    FLOAT,
    DOUBLE,
    LONG,
};

typedef cstr_t unique_id_t;

class Type
{
private:
    union
    {
        EPrimitiveId Primitive;
        unique_id_t Unique;
    } m_Id;

    bool m_bPrimitive;

    Type(bool bPrimitive) :
        m_Id(),
        m_bPrimitive(bPrimitive)
    {
    }

public:
    Type(EPrimitiveId primitive) :
        Type(true)
    {
        m_Id.Primitive = primitive;
    }

    Type(unique_id_t unique) :
        Type(false)
    {
        m_Id.Unique= unique;
    }

    EPrimitiveId GetPrimitive() const
    {
        return m_bPrimitive ? m_Id.Primitive : EPrimitiveId::UNKNOWN;
    }

    unique_id_t GetUnique() const
    {
        return !m_bPrimitive ? m_Id.Unique : NULL;
    }

    bool Equals(EPrimitiveId primitive) const
    {
        return m_bPrimitive && m_Id.Primitive == primitive;
    }

    bool Equals(unique_id_t unique) const
    {
        return !m_bPrimitive && m_Id.Unique == unique;
    }

    bool operator==(EPrimitiveId primitive) const
    {
        return Equals(primitive);
    }

    bool operator==(unique_id_t unique) const
    {
        return Equals(unique);
    }
};

#define INT_CSTR "int"

Type ParseType(cstr_t typeStr)
{
    if (!strcmp(typeStr, INT_CSTR))
        return Type(EPrimitiveId::INT);

    return Type(typeStr);
}

class Field
{
protected:
    cstr_t m_Name;
    Type m_Type;
    EFieldAccess m_Access;
    bool m_bStatic;

    Field(cstr_t name, Type type, EFieldAccess access, bool bStatic) :
        m_Name(name),
        m_Type(type),
        m_Access(access),
        m_bStatic(bStatic)
    {
    }

    Field(cstr_t name, cstr_t typeStr, cstr_t accessStr, bool bStatic) :
        m_Name(name),
        m_Type(ParseType(typeStr)),
        m_Access(ParseAccess(accessStr)),
        m_bStatic(bStatic)
    {
    }

public:
    cstr_t GetName() const
    {
        return m_Name;
    }

    EFieldAccess GetAccess() const
    {
        return m_Access;
    }

    const Type &GetType() const
    {
        return m_Type;
    }

    virtual unk_t GetWeak(unk_t object) const = 0;

    template <typename T>
    T *Get(unk_t object) const
    {
        return reinterpret_cast<T *>(GetWeak(object));
    }
};

class Class
{
private:
    cstr_t m_Name;
    std::vector<unk_t> m_Instances;
    std::vector<Field *> m_Fields;

public:
    Class(cstr_t name) :
        m_Name(name),
        m_Instances(),
        m_Fields()
    {
    }

    void AddInstance(unk_t instance)
    {
        m_Instances.push_back(instance);
    }

    void AddField(Field *field)
    {
        m_Fields.push_back(field);
    }

    cstr_t GetName() const
    {
        return m_Name;
    }

    const std::vector<Field *> &GetFields() const
    {
        return m_Fields;
    }

    const Field *GetField(cstr_t fieldName) const
    {
        for (const Field *field : m_Fields)
            if (!strcmp(fieldName, field->GetName()))
                return field;

        return NULL;
    }
};

std::vector<Class *> g_ClassDescriptors;

Class *CreateClassDescriptor(cstr_t className)
{
    Class *newClass = new Class(className);
    g_ClassDescriptors.push_back(newClass);
    return newClass;
}

namespace hidden
{
}

#define _RFLCT_CLASS_DEFINE_CONTAINER(className) \
namespace hidden \
{ \
    class className##Container \
    { \
    private: \
        static Class *const s_##className##Class; \
        \
    protected: \
        className##Container() \
        { \
            s_##className##Class->AddInstance(this); \
        } \
        \
        void AddField(Field *field) \
        { \
            s_##className##Class->AddField(field); \
        } \
    \
    public: \
        const Class *GetClass() \
        { \
            return s_##className##Class; \
        } \
    }; \
    \
    Class *const className##Container::s_##className##Class = CreateClassDescriptor(#className); \
}

#define RFLCT_CLASS(className) \
_RFLCT_CLASS_DEFINE_CONTAINER(className) \
class className : public hidden::className##Container

#define RFLCT_CHILD_CLASS(className, ...) \
_RFLCT_CLASS_DEFINE_CONTAINER(className) \
class className : public hidden::className##Container, __VA_ARGS__

#define RFLCT_FIELD(className, access, type, name) \
class name##Field : public Field \
{ \
public: \
    name##Field() : \
        Field(#name, #type, #access, false) \
    { \
    } \
    \
    unk_t GetWeak(unk_t object) const override \
    { \
        return &((className *) object)->name; \
    } \
}; \
\
class name##FieldCreator \
{ \
private: \
    inline static bool s_IsDefined = false; \
\
public: \
    name##FieldCreator(className *object) \
    { \
        if (s_IsDefined) \
            return; \
        \
        object->AddField(new name##Field()); \
        \
        s_IsDefined = true; \
    } \
} name##FieldInst = name##FieldCreator(this); \
\
access: \
    type name

RFLCT_CLASS(Object)
{
    RFLCT_FIELD(Object, public, int, m_Id);

public:
    Object(int id) :
        m_Id(id)
    {
    }
};

int main()
{
    Object obj(5);

    std::cout << obj.m_Id << std::endl;

    for (Field *field : obj.GetClass()->GetFields())
    {
        field->GetType();
    }

    std::cout << obj.m_Id << std::endl;
}
