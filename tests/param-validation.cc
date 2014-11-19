#include <bandit/bandit.h>
#include <gcm/json/validator.h>

using namespace bandit;

using namespace gcm::json;
namespace v = gcm::json::validator;

// vtable for json::Value goes here.
Value::~Value()
{}

go_bandit([](){
    describe("json", [](){
        describe("validator", [](){
            it("succeeds basic validation", [](){
                Array array;
                array.push_back(make_int(123));
                array.push_back(make_string("hello"));

                auto def = v::ParamDefinitions(
                    v::Int("param1", "Test param 1"),
                    v::String("param2", "Test param 2")
                );

                AssertThat(def.validate(array), Equals(true));
            });

            it("throws when invalid argument passed", [](){
                Array array;
                array.push_back(make_int(123));
                array.push_back(make_string("hello"));

                auto def = v::ParamDefinitions(
                    v::String("param1", "Test param 1"),
                    v::Int("param2", "Test param 2")
                );

                AssertThrows(v::Diagnostics, def.validate(array));
            });

            it("throws when too many arguments for function are passed", [](){
                Array array;
                array.push_back(make_int(123));
                array.push_back(make_string("hello"));
                array.push_back(make_string("world"));

                auto def = v::ParamDefinitions(
                    v::Int("param1", "Test param 1"),
                    v::String("param2", "Test param 2")
                );

                AssertThrows(v::Diagnostics, def.validate(array));
            });

            it("throws when too few arguments for function are passed", [](){
                Array array;
                array.push_back(make_int(123));

                auto def = v::ParamDefinitions(
                    v::Int("param1", "Test param 1"),
                    v::String("param2", "Test param 2")
                );

                AssertThrows(v::Diagnostics, def.validate(array));
            });

            it("passes on null", [](){
                Array array;
                array.push_back(make_null());

                auto def = v::ParamDefinitions(
                    v::Nullable(v::Int("param1", "Test param 1"))
                );

                AssertThat(def.validate(array), Equals(true));
            });

            it("validates object", [](){
                Array array;
                array.push_back(make_object(
                    std::make_pair("param1", make_string("String")),
                    std::make_pair("param2", make_int(123))
                ));

                auto def = v::ParamDefinitions(
                    v::Object("params", "Param definitions",
                        v::String("param1", "String param"),
                        v::Int("param2", "Int param")
                    )
                );

                AssertThat(def.validate(array), Equals(true));
            });

            it("throws when item is missing in object", [](){
                Array array;
                array.push_back(make_object(
                    std::make_pair("param1", make_string("String"))
                ));

                auto def = v::ParamDefinitions(
                    v::Object("params", "Params definitions",
                        v::String("param1", "String param"),
                        v::Int("param2", "Int param")
                    )
                );

                AssertThrows(v::Diagnostics, def.validate(array));
            });

            it("throws when item is in object but not in definition", [](){
                Array array;
                array.push_back(make_object(
                    std::make_pair("param1", make_string("Hello")),
                    std::make_pair("param2", make_string("World!"))
                ));

                auto def = v::ParamDefinitions(
                    v::Object("params", "Param defitinions",
                        v::String("param1", "The only param")
                    )
                );

                AssertThrows(v::Diagnostics, def.validate(array));
            });

        });
    });
});
