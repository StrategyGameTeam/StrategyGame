#pragma once
#include <unordered_map>
#include "hex.hpp"

struct BaseUnitData {
    int id;
    int health;
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

    const auto& get_all_on_hex(HexCoords hc) {
        return m_store[hc]; // maybe return optional instead of just constructing?
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
};
