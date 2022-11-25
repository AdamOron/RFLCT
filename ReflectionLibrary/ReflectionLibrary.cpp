#include <iostream>
#include <vector>

using cstr_t = const char *;
using unk_t = void *;
using cinstance_t = unk_t;
using type_t = const std::type_info &;

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

class Field
{
protected:
    cstr_t m_Name;
    const std::type_info &m_Type;
    EFieldAccess m_Access;
    bool m_bStatic;

    Field(cstr_t name, const std::type_info &type, EFieldAccess access, bool bStatic) :
        m_Name(name),
        m_Type(type),
        m_Access(access),
        m_bStatic(bStatic)
    {
    }

    Field(cstr_t name, const std::type_info &type, cstr_t accessStr, bool bStatic) :
        m_Name(name),
        m_Type(type),
        m_Access(ParseAccess(accessStr)),
        m_bStatic(bStatic)
    {
    }

    virtual unk_t GetAbstractPointer(unk_t object) const = 0;

public:
    cstr_t GetName() const
    {
        return m_Name;
    }

    EFieldAccess GetAccess() const
    {
        return m_Access;
    }

    const std::type_info &GetType() const
    {
        return m_Type;
    }

    template <typename T>
    T GetValue(unk_t object) const
    {
        return *reinterpret_cast<T *>(GetAbstractPointer(object));
    }

    template <typename T>
    void SetValue(unk_t object, T value)
    {
        *reinterpret_cast<T *>(GetAbstractPointer(object)) = value;
    }
};

class Function
{
private:
    unk_t m_pFunction;
    type_t m_ReturnType;
    std::vector<type_t> m_ArgTypes;

public:
    Function(unk_t pFunction, type_t returnType) :
        m_pFunction(pFunction),
        m_ReturnType(returnType),
        m_ArgTypes()
    {
    }
};

class Class
{
private:
    cstr_t m_Name;
    std::vector<cinstance_t> m_Instances;
    std::vector<Field *> m_Fields;

public:
    Class(cstr_t name) :
        m_Name(name),
        m_Instances(),
        m_Fields()
    {
    }

    void AddInstance(cinstance_t instance)
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

    const std::vector<cinstance_t> &GetInstances() const
    {
        return m_Instances;
    }

    Field *GetField(cstr_t fieldName) const
    {
        for (Field *field : m_Fields)
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

namespace RFLCT
{
}

#define _RFLCT_CLASS_DEFINE_CONTAINER(className) \
namespace _hidden_namespace \
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
class className : public _hidden_namespace::className##Container

#define RFLCT_CHILD_CLASS(className, ...) \
_RFLCT_CLASS_DEFINE_CONTAINER(className) \
class className : public _hidden_namespace::className##Container, __VA_ARGS__

#define RFLCT_FIELD(className, access, type, name) \
class name##Field : public Field \
{ \
public: \
    name##Field() : \
        Field(#name, typeid(type), #access, false) \
    { \
    } \
    \
protected: \
    unk_t GetAbstractPointer(unk_t object) const override \
    { \
        return &((className *) object)->name; \
    } \
}; \
\
private: \
bool _##name##FieldCreator = [this] \
{ \
    static bool s_bFieldDefined = false; \
    \
    if (s_bFieldDefined) \
        return false; \
    \
    this->AddField(new name##Field()); \
    \
    s_bFieldDefined  = true; \
    \
    return true; \
}(); \
\
access: \
    type name

#define RFLCT_FUNCTION(className, access, name, returnType, ...) \
access: \
    returnType name(__VA_ARGS__)

#define A(...) typeid(__VA_ARGS__)

RFLCT_CLASS(Object)
{
    RFLCT_FIELD(Object, private, int, id);
    RFLCT_FIELD(Object, protected, int, count);
    RFLCT_FIELD(Object, public, cstr_t, name);

public:
    Object(int id) :
        id(id),
        count(0),
        name("dick")
    {
    }
};

int main()
{
    Object a(5), b(10), c(15);

    const Class *clazz = a.GetClass();

    for (Field *field : clazz->GetFields())
        printf("%s %d\n", field->GetName(), field->GetAccess());

    Field *field = clazz->GetField("id");
    for (cinstance_t inst : a.GetClass()->GetInstances())
        printf("%d\n", field->GetValue<int>(inst));
}
