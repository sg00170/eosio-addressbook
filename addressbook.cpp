#include <eosio/eosio.hpp>
#include "abcounter/abcounter.cpp"

// namespace
using namespace eosio;

// 클래스 선언 (class declaration)
// 상속 (inheritance) // class 파생클래스명 : 상속접근지정 기본클래스명 {};
// class                                    파생클래스명  : 상속접근지정 
class [[ eosio::contract("addressbook") ]] addressbook : public
// 기본클래스명 {};
eosio::contract {
    // (access specifier - 접근지정자) 클래스 내외의 모든 함수에게 접근 허용
    public: 
        // constructor
        addressbook(name receiver, name code, datastream<const char*> ds):contract(receiver, code, ds) {}

        [[ eosio::action ]]
        // 등록 & 업데이트 함수
        void upsert (
            name user,
            std::string first_name,
            std::string last_name,
            uint64_t age,
            std::string street,
            std::string city,
            std::string state
        ) {
            require_auth(user);
            // get_self : 사용자 / get_first_receiver : 배포계정
            address_index addresses(get_self(), get_first_receiver().value);
            auto iterator = addresses.find(user.value);
            if(iterator == addresses.end()) {
                // The user isn't in the table
                                        // 람다(Lamda)함수 - 익명함수
                addresses.emplace(user, [&](auto& row) {
                                                // & : 참조
                    row.key = user;
                    row.first_name = first_name;
                    row.last_name = last_name;
                    row.age = age;
                    row.street = street;
                    row.city = city;
                    row.state = state;
                });

                send_summary(user, "successfully emplaced record to addressbook");
                increment_counter(user, "emplace");
            }
            else {
                std::string changes;
                // The user is in the table
                addresses.modify(iterator, user, [&](auto& row) {
                    if(row.first_name != first_name) {
                        row.first_name = first_name;
                        changes += "first name ";
                    }
                    if(row.last_name != last_name) {
                        row.last_name = last_name;
                        changes += "last name ";
                    }
                    if(row.age != age) {
                        row.age = age;
                        changes += "age ";
                    }
                    if(row.street != street) {
                        row.street = street;
                        changes += "street ";
                    }
                    if(row.city != city) {
                        row.city = city;
                        changes += "city ";
                    }
                    if(row.state != state) {
                        row.state = state;
                        changes += "state ";
                    }
                });

                if(!changes.empty()) {
                    send_summary(user, " successfully modified record in addressbook. Fields changed: " + changes);
                    increment_counter(user, "modify");
                }
                else {
                    send_summary(user, " called upsert, but request resulted in no changes.");
                }
            }
        }

        [[ eosio::action ]]
        // 삭제 함수
        void erase (name user) {
            require_auth(user);
            address_index addresses(get_self(), get_first_receiver().value);
            auto iterator = addresses.find(user.value);
            check(iterator != addresses.end(), "Record does not exists");
            addresses.erase(iterator);

            send_summary(user, "successfully erased record from addressbook");
            increment_counter(user, "erase");
        }

        [[ eosio::action ]]
        // notify 함수
        void notify(name user, std::string msg) {
            require_auth(get_self());
            require_recipient(user);
        }
    // (access specifier - 접근지정자) 클래스 내의 멤버 함수만 접근 허용
    private:
        // Data structure
        struct [[ eosio::table ]] person {
            name key;
            uint64_t age;
            std::string first_name;
            std::string last_name;
            std::string street;
            std::string city;
            std::string state;

            uint64_t primary_key() const { return key.value; } // name type의 value는 uint64_t로 변환된 값
            uint64_t get_secondary_1() const { return age; }
        };

        // Inline action by internal contracts
        void send_summary(name user, std::string message) {
            action(
                permission_level{get_self(), "active"_n}, // permission level(permission_level struct)
                get_self(), // code (eosio::name type)
                "notify"_n, // action (eosio::name type)
                std::make_tuple(user, name{user}.to_string() + message) // data
            ).send();
        };

        // Inline action by external contracts (abcounter)
        void increment_counter(name user, std::string type) {
            // 객체 초기화
                                        // 수신자 계약 이름   // 권한 구조체
            abcounter::count_action count("abcounter"_n, {get_self(), "active"_n});
            count.send(user, type);
        };

        // multi index
        typedef eosio::multi_index<
            "people"_n, person,
            // indexed_by structure
                    // 이름     // 함수 호출 연산자
            indexed_by<"byage"_n, const_mem_fun<person, uint64_t, &person::get_secondary_1>>
        > address_index;
        
        // using address_index = eosio::multi_index<
        // "people"_n, person, 
        // // indexed_by structure
        //             // 이름     // 함수 호출 연산자
        // indexed_by<"byage"_n, const_mem_fun<person, uint64_t, &person::get_secondary_1>>
        // >;
};