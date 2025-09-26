#pragma once
#include <nlohmann/json.hpp>

using Eigen::Vector3d;
using json = nlohmann::json;

class ImageSequence {
public:
    ImageSequence() = default;
    ImageSequence(std::string filename, int n_images);
    json to_json() const;
protected:
    int n_images_ {};
    std::string filename_ {};
    std::vector<std::size_t> single_file_indices_{};
};

ImageSequence::ImageSequence(std::string filename, int n_images) : filename_(filename), n_images_(n_images) {
    single_file_indices_.reserve(n_images_);
    for (std::size_t i=0;i<n_images_;++i){
        single_file_indices_.push_back(i);
    }
}

json ImageSequence::to_json() const {
    json imageset_data;
    imageset_data["__id__"] = "ImageSequence";
    imageset_data["template"] = filename_;
    imageset_data["single_file_indices"] = single_file_indices_;
    // Set defaults and null for now.
    imageset_data["mask"] = nullptr;
    imageset_data["gain"] = nullptr;
    imageset_data["pedestal"] =nullptr;
    imageset_data["dx"] = nullptr;
    imageset_data["dy"] = nullptr;
    json params;
    params["dynamic_shadowing"] = "Auto";
    params["multi_panel"] = false;
    imageset_data["params"] = params;
    return imageset_data;
}