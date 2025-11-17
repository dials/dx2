#pragma once
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class ImageSequence {
public:
    ImageSequence() = default;
    ImageSequence(std::string filename, int n_images); // Constructor for MultiImage formats e.g. h5.
    ImageSequence(std::string filename); // Constructor for non-MultiImage formats e.g. cbf.
    ImageSequence(json imagesequence_data);
    json to_json() const;
protected:
    int n_images_ {};
    std::string filename_ {};
    std::vector<std::size_t> single_file_indices_{};
    json imagesequence_data_ {}; // For propagating additional metadata during serialization/deserialization.
};