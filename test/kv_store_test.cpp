#include "kv_store.h"
#include <iostream>
#include <cassert>

int main() {
    // tests will go here
    KVStore kv;
    kv.apply(PutCommand{"name", "alice"});
    auto val = kv.get("name");
    assert(val.has_value());
    assert(val.value() == "alice");
    std::cout<<"PUT/GET: OK\n";

    auto val2 = kv.get("DOB");
    assert(!val2.has_value());
    std::cout<<"GET: on no key - OK\n";

    kv.apply(DeleteCommand{"name"});
    assert(!kv.get("name").has_value());
    std::cout<<"DELETE: OK\n";

    auto result = kv.apply(DeleteCommand{"ghost"});
    assert(result.success);
    std::cout << "DELETE idempotency: OK\n";

    std::cout << "All tests passed.\n";
    return 0;
}