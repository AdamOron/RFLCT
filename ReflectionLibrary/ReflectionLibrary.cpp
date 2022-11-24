#include <iostream>
#include <vector>

using cstr_t = const char *;
using unk_t = void *;

#define PUBLIC_CSTR      "public"
#define PROTECTED_CSTR   "protected"
#define PRIVATE_CSTR     "private"

enum class FieldAccess
{
    PUBLIC,
    PROTECTED,
    PRIVATE,
    UNKNOWN,
};

FieldAccess ParseAccess(cstr_t accessStr)
{
    if (!strcmp(accessStr, PUBLIC_CSTR))
        return FieldAccess::PUBLIC;

    if (!strcmp(accessStr, PROTECTED_CSTR))
        return FieldAccess::PROTECTED;

    if (!strcmp(accessStr, PRIVATE_CSTR))
        return FieldAccess::PRIVATE;

    return FieldAccess::UNKNOWN;
}

class Field
{
protected:
    cstr_t m_Name;
    FieldAccess m_Access;
    bool m_bStatic;

    Field(cstr_t name, FieldAccess access, bool bStatic) :
        m_Name(name),
        m_Access(access),
        m_bStatic(bStatic)
    {
    }

    Field(cstr_t name, cstr_t accessStr, bool bStatic) :
        m_Name(name),
        m_Access(ParseAccess(accessStr)),
        m_bStatic(bStatic)
    {
    }

public:
    cstr_t GetName() const
    {
        return m_Name;
    }

    FieldAccess GetAccess() const
    {
        return m_Access;
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
    name##Field(cstr_t name, cstr_t accessStr) : \
        Field(name, accessStr, false) \
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
        object->AddField(new name##Field(#name, #access)); \
        \
        s_IsDefined = true; \
    } \
} \
name##FieldInst = name##FieldCreator(this); \
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

    for (Class *clazz : g_ClassDescriptors)
    {
        if (!strcmp(clazz->GetName(), "Object"))
        {
            int *pId = clazz->GetField("m_Id")->Get<int>(&obj);
            std::cout << *pId << std::endl;
            *pId = 7;
        }
    }

    std::cout << obj.m_Id << std::endl;
}
