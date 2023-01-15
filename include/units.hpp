#pragma once
#include <unordered_map>
#include "hex.hpp"

struct BaseUnitData {
    int id;
    int fraction;
    int health;
    int vission_range = 1;
    int stamina = 100;
    std::string owner;
};

struct MilitaryUnit : public BaseUnitData {
};

struct CivilianUnit : public BaseUnitData {
};

struct SpecialUnit : public BaseUnitData {
};

enum class UnitType {
    Unspecified = 0,
    Millitary,
    Civilian,
    Special
};

struct UnitsOnTile {
    std::optional<MilitaryUnit> military;
    std::optional<CivilianUnit> civilian;
    std::optional<SpecialUnit> special;
    
    bool has_any () const {
        return military.has_value() || civilian.has_value() || special.has_value();
    }

    std::string getOwner() const {
        if(military.has_value()){
            return military.value().owner;
        }
        if(civilian.has_value()){
            return civilian.value().owner;
        }
        if(special.has_value()){
            return special.value().owner;
        }
        return "";
    }

    int getStamina() const {
        if(military.has_value()){
            return military.value().stamina;
        }
        if(civilian.has_value()){
            return civilian.value().stamina;
        }
        if(special.has_value()){
            return special.value().stamina;
        }
        return 0;
    }

    void addStamina(int stamina) {
        if(military.has_value()){
            military.value().stamina += stamina;
        }
        if(civilian.has_value()){
            civilian.value().stamina += stamina;
        }
        if(special.has_value()){
            special.value().stamina += stamina;
        }
    }

    bool isCurrentUser(std::string &nickname) const {
        return getOwner() == nickname;
    }

    template <UnitType Type>
    auto& get_opt_unit() = delete;

    template <> auto& get_opt_unit<UnitType::Millitary> () { return military; };
    template <> auto& get_opt_unit<UnitType::Civilian> () { return civilian; };
    template <> auto& get_opt_unit<UnitType::Special> () { return special; };
};

struct UnitStore {
    // NOTE: This may be a big performance hit, since we want reference stability, and fast iteration
    // for now, it's done kinda lazyli
    std::unordered_map<HexCoords, UnitsOnTile> m_store; // maybe we dont need arbitrarily many units in a single spot?

    void call_this_when_unit_moves(decltype(m_store.begin()) moved_it, HexCoords new_location) {
        auto node = m_store.extract(moved_it);
        node.key() = new_location;
        m_store.insert(std::move(node));
    }

    auto& get_all_on_hex(HexCoords hc) {
        return m_store[hc]; // maybe return optional instead of just constructing?
    }

    bool is_selection_valid (std::pair<HexCoords, UnitType> selection) const {
        auto [coord, type] = selection;
        if (auto it = m_store.find(coord); it != m_store.end()) {
            switch (type) {
                case UnitType::Millitary:return it->second.military.has_value();
                case UnitType::Civilian: return it->second.civilian.has_value();
                case UnitType::Special: return it->second.special.has_value();
                default:
                case UnitType::Unspecified: return false;
            }
        } else {
            return false;
        }
    }

    // Returns true if putting the unit on the hex was successful, false if there was something there already 
    bool put_unit_on_hex(HexCoords hc, MilitaryUnit unit) {
        // operator[] constructs if not present
        if (m_store[hc].military.has_value()) {
            return false;
        } else {
            m_store[hc].military = unit;
            return true;
        }
    }

    // Returns true if putting the unit on the hex was successful, false if there was something there already 
    bool put_unit_on_hex(HexCoords hc, CivilianUnit unit) {
        // operator[] constructs if not present
        if (m_store[hc].civilian.has_value()) {
            return false;
        } else {
            m_store[hc].civilian = unit;
            return true;
        }
    }
    
    // Returns true if putting the unit on the hex was successful, false if there was something there already 
    bool put_unit_on_hex(HexCoords hc, SpecialUnit unit) {
        // operator[] constructs if not present
        if (m_store[hc].special.has_value()) {
            return false;
        } else {
            m_store[hc].special = unit;
            return true;
        }
    }


    void hard_override_unit_on_hex(HexCoords hc, MilitaryUnit unit) {
        m_store[hc].military = unit;
    }

    void hard_override_unit_on_hex(HexCoords hc, CivilianUnit unit) {
        m_store[hc].civilian = unit;
    }

    void hard_override_unit_on_hex(HexCoords hc, SpecialUnit unit) {
        m_store[hc].special = unit;
    }

    void teleport_unit_militray (HexCoords start, HexCoords end) {
        m_store[end].military = m_store[start].military;
        m_store[start].military.reset();
    }
    void teleport_unit_civilian (HexCoords start, HexCoords end) {
        m_store[end].civilian = m_store[start].civilian;
        m_store[start].civilian.reset();
    }
    void teleport_unit_special (HexCoords start, HexCoords end) {
        m_store[end].special = m_store[start].special;
        m_store[start].special.reset();
    }

    template <UnitType Type>
    void teleport_unit (HexCoords start, HexCoords end) {
        if (start == end) return;
        m_store[end].get_opt_unit<Type>() = m_store[start].get_opt_unit<Type>();
        m_store[start].get_opt_unit<Type>().reset();
    }
};
