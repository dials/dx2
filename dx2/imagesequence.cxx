#include "dx2/imagesequence.hpp"

using json = nlohmann::json;

// Constructor for MultiImage formats e.g. h5.
ImageSequence::ImageSequence(std::string filename, int n_images) : filename_(filename), n_images_(n_images) {
    single_file_indices_.reserve(n_images_);
    for (std::size_t i=0;i<n_images_;++i){
        single_file_indices_.push_back(i);
    }
}

// Constructor for non-MultiImage formats e.g. cbf.
ImageSequence::ImageSequence(std::string filename) : filename_(filename) {}

ImageSequence::ImageSequence(json imagesequence_data) {
    std::vector<std::string> required_keys = {"template", "__id__"};
    for (const auto &key : required_keys) {
        if (imagesequence_data.find(key) == imagesequence_data.end()) {
            throw std::invalid_argument("Key " + key +
                                       " is missing from the input imageset JSON");
        }
    }
    if (imagesequence_data["__id__"] != std::string("ImageSequence")){
        throw std::runtime_error("Only ImageSequences are supported");
    }
    filename_ = imagesequence_data["template"];
    if (imagesequence_data.find("single_file_indices") != imagesequence_data.end()) {
        // for non-multimage formats (e.g. non-h5), allow parsing.
        json indices = imagesequence_data["single_file_indices"];
        single_file_indices_ = {};
        if (*(indices.begin()) < 0){
            throw std::runtime_error("Starting file index <0");
        }
        for (json::iterator it = indices.begin(); it != indices.end();
            ++it) {
            single_file_indices_.push_back(*it);
        }
        n_images_ = single_file_indices_.size();
    }
    imagesequence_data_ = imagesequence_data; // To propagate during serialization/deserialization.
}

json ImageSequence::to_json() const {
    json imageset_data = imagesequence_data_;
    imageset_data["__id__"] = "ImageSequence";
    imageset_data["template"] = filename_;
    if (single_file_indices_.size() > 0){ // i.e. MultiImage formats (h5).
        imageset_data["single_file_indices"] = single_file_indices_;
    }
    // Set defaults and null for now.
    std::vector<std::string> optional_keys = {"mask", "gain", "pedestal", "dx", "dy"};
    for (const auto &key : optional_keys) {
        if (imagesequence_data_.find(key) == imagesequence_data_.end()) {
            imageset_data[key] = nullptr;
        }
    }
    if (imagesequence_data_.find("params") == imagesequence_data_.end()) {
        json params;
        params["dynamic_shadowing"] = "Auto";
        params["multi_panel"] = false;
        imageset_data["params"] = params;
    }
    return imageset_data;
}