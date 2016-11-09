#include <gflags/gflags.h>

#include "drake/common/drake_path.h"
#include "drake/common/text_logging.h"
#include "drake/lcm/drake_lcm.h"
#include "drake/multibody/parser_sdf.h"
#include "drake/multibody/parser_urdf.h"
#include "drake/multibody/rigid_body_tree_construction.h"
#include "drake/multibody/rigid_body_plant/drake_visualizer.h"
#include "drake/multibody/rigid_body_plant/rigid_body_plant.h"
#include "drake/systems/analysis/simulator.h"
#include "drake/systems/framework/diagram.h"
#include "drake/systems/framework/diagram_builder.h"
#include "drake/systems/framework/primitives/constant_vector_source.h"

namespace drake {
namespace examples {
namespace quadrotor {
namespace {

DEFINE_double(duration, 3, "Total duration of simulation.");

template <typename T>
class Quadrotor : public systems::Diagram<T> {
 public:
  Quadrotor() {
    this->set_name("Quadrotor");

    auto tree = std::make_unique<RigidBodyTree<T>>();

    drake::parsers::urdf::AddModelInstanceFromUrdfFile(
        drake::GetDrakePath() + "/examples/Quadrotor/quadrotor.urdf",
        multibody::joints::kRollPitchYaw, nullptr, tree.get());

    drake::parsers::sdf::AddModelInstancesFromSdfFile(
        drake::GetDrakePath() + "/examples/Quadrotor/warehouse.sdf",
        multibody::joints::kFixed, nullptr, tree.get());

    drake::multibody::AddFlatTerrainToWorld(tree.get());

    systems::DiagramBuilder<T> builder;

    plant_ =
        builder.template AddSystem<systems::RigidBodyPlant<T>>(std::move(tree));

    VectorX<T> hover_input(plant_->get_input_size());
    hover_input.setZero();
    systems::ConstantVectorSource<T>* source =
        builder.template AddSystem<systems::ConstantVectorSource<T>>(
            hover_input);

    systems::DrakeVisualizer* publisher =
        builder.template AddSystem<systems::DrakeVisualizer>(
            plant_->get_rigid_body_tree(), &lcm_);

    builder.Connect(source->get_output_port(), plant_->get_input_port(0));
    builder.Connect(plant_->get_output_port(0), publisher->get_input_port(0));

    builder.BuildInto(this);
  }

  void SetDefaultState(systems::Context<T>* context) const {
    systems::Context<T>* plant_context =
        this->GetMutableSubsystemContext(context, plant_);
    VectorX<T> x0(plant_->get_num_states());
    x0.setZero();
    /* x0 is the initial state where
     * x0(0), x0(1), x0(2) are the quadrotor's x, y, z -states
     * x0(3), x0(4), x0(5) are the quedrotor's Euler angles phi, theta, psi
     */
    x0(2) = 0.2;  // setting arbitrary z-position
    plant_->set_state_vector(plant_context, x0);
  }

 private:
  systems::RigidBodyPlant<T>* plant_{};
  lcm::DrakeLcm lcm_;
};

int do_main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  Quadrotor<double> model;
  systems::Simulator<double> simulator(model);

  model.SetDefaultState(simulator.get_mutable_context());

  simulator.Initialize();
  simulator.StepTo(FLAGS_duration);
  return 0;
}

}  // namespace
}  // namespace quadrotor
}  // namespace examples
}  // namespace drake

int main(int argc, char* argv[]) {
  return drake::examples::quadrotor::do_main(argc, argv);
}
