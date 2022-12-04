#pragma once
#include <vector>
#include "hex.hpp"

struct RenderingController {

    std::vector<HexCoords> all_within_unscaled_quad (
        Vector2 top_left, Vector2 top_right, Vector2 bottom_left, Vector2 bottom_right
    ) {
        std::vector<HexCoords> line = ([&]{
            std::vector<HexCoords> result;
            // start from the top right
            const auto bl = HexCoords::from_world_unscaled(bottom_left.x, bottom_left.y) + 1_LD; 
            const auto br = HexCoords::from_world_unscaled(bottom_right.x, bottom_right.y) + 1_RD;
            const auto steps = bl.distance(br);
            result.reserve(steps+1);
            result.push_back(bl);
            for(int i = 0; i < steps; i++) {
                result.push_back(result.back() + 1_R);
            }
            return result;
        })();
        
        std::vector<HexCoords> result;
        result.insert(result.end(), line.begin(), line.end());

        const auto limit_r = HexCoords::from_world_unscaled(top_left.x, top_left.y).r - 1;

        while(line.front().r >= limit_r) {
            std::vector<HexCoords> local_line;
            local_line.reserve(line.size() + 2);
            for(size_t i = 0; i < line.size(); i += 2) {
                local_line.push_back(line.at(i) + 1_LU);
                local_line.push_back(line.at(i) + 1_RU);
            }
            if (line.size() % 2 == 0) {
                local_line.push_back(local_line.back() + 1_R);
            }
            result.insert(result.end(), local_line.begin(), local_line.end());
            line = local_line;
        }

        return result;
    }
};