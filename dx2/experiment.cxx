#include <dx2/beam_io.hpp>
#include <dx2/experiment.hpp>

Experiment::Experiment(Models models, std::string identifier)
   : _models(std::move(models)), _identifier(std::move(identifier)){
    if (_identifier.empty()) {
      generate_identifier();
    }
   }

Crystal& Experiment::crystal() {
  if (!_models.crystal) {
    throw std::runtime_error("Experiment has no crystal");
  }
  return *_models.crystal;
}
const Crystal& Experiment::crystal() const {
  if (!_models.crystal) {
    throw std::runtime_error("Experiment has no crystal");
  }
  return *_models.crystal;
}

void Experiment::set_crystal(Crystal crystal) { _models.crystal = std::make_shared<Crystal>(std::move(crystal)); }

void Experiment::set_crystal(std::shared_ptr<Crystal> crystal){_models.crystal = std::move(crystal);}

void Experiment::generate_identifier() { _identifier = ersatz_uuid4(); }
