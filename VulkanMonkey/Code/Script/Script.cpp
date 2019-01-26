#include "Script.h"
#include <iostream>

using namespace vm;

MonoDomain* Script::monoDomain = nullptr;
bool Script::initialized = false;

constexpr uint32_t PUBLIC_FLAG = 0x0006;

Script::Script(std::string file, std::string extension)
{
	ctor = nullptr;
	dtor = nullptr;
	update = nullptr;
	assembly = mono_domain_assembly_open(monoDomain, std::string("Scripts/" + file + "." + extension).c_str()); // "Scripts/file.extension"
	monoImage = mono_assembly_get_image(assembly);
	entityClass = mono_class_from_name(monoImage, "", file.c_str());
	entityInstance = mono_object_new(monoDomain, entityClass);
	mono_runtime_object_init(entityInstance);

	// variables
	void* iter = NULL;
	while (MonoClassField* f = mono_class_get_fields(entityClass, &iter))
		fields.push_back(f);

	//for (auto& f : fields) {
	//	if (mono_field_get_flags(f) & PUBLIC_FLAG) { // get only public fields
	//		std::string fieldName(mono_field_get_name(f));
	//		uint32_t mtype = mono_type_get_type(mono_field_get_type(f));
	//		if (fieldName == "transform" && mtype & MONO_TYPE_CLASS)
	//				transform = f;
	//	}
	//}

	// functions
	void* it = NULL;
	while (MonoMethod* m = mono_class_get_methods(entityClass, &it))
		methods.push_back(m);

	for (auto& m : methods) {
		if (mono_method_get_flags(m, nullptr) & PUBLIC_FLAG) {
			std::string methodName(mono_method_get_name(m));
			if (methodName == ".ctor") {
				void** args = nullptr;
				MonoObject* exception = nullptr;
				//mono_runtime_invoke(m, entityInstance, args, &exception);
				ctor = m;
			}
			else if (methodName == "Finalize")
				dtor = m;
			else if (methodName == "Update")
				update = m;
		}
	}
}

Script::~Script()
{
	if (!dtor) return;
	void** args = nullptr;
	MonoObject* exception = nullptr;
	mono_runtime_invoke(dtor, entityInstance, args, &exception);
}

void Script::Init()
{
	if (initialized)
		return;
	mono_set_dirs("include/Mono/lib", "include/Mono/etc");
	mono_config_parse(nullptr);
	monoDomain = mono_jit_init("VMonkey");

	initialized = true;
}

void Script::Cleanup()
{
	mono_jit_cleanup(monoDomain);
}

void Script::addCallback(const char * target, const void * staticFunction)
{
	mono_add_internal_call(target, staticFunction);
}

void Script::Update(float delta)
{
	if (!update) return;
	void* args[1];
	args[0] = &delta;
	MonoObject* exception = NULL;
	mono_runtime_invoke(update, entityInstance, args, &exception);
}