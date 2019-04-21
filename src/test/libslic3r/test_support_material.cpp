#include <catch.hpp>
#include <libslic3r/IO.hpp>
#include <libslic3r/GCodeReader.hpp>

#include "libslic3r.h"
#include "TriangleMesh.hpp"
#include "Model.hpp"
#include "SupportMaterial.hpp"
#include "test_data.hpp"
#include "Log.hpp"

using namespace std;
using namespace Slic3r;

void test_1_checks(Print &print, bool &a, bool &b, bool &c, bool &d);
bool test_6_checks(Print &print);

// Testing 0.1: supports material member functions.
TEST_CASE("", "")
{
    // Create a mesh & modelObject.
    TriangleMesh mesh = TriangleMesh::make_cube(20, 20, 20);

    // Create modelObject.
    Model model = Model();
    ModelObject *object = model.add_object();
    object->add_volume(mesh);
    model.add_default_instances();

    // Align to origin.
    model.align_instances_to_origin();

    // Create Print.
    Print print = Print();
    vector<coordf_t> contact_z = {1.9};
    vector<coordf_t> top_z = {1.1};
    print.default_object_config.support_material = 1;
    print.default_object_config.set_deserialize("raft_layers", "3");
    print.add_model_object(model.objects[0]);
    print.objects.front()->_slice();


    SupportMaterial *support = print.objects.front()->_support_material();

    support->generate(print.objects.front());
    REQUIRE(print.objects.front()->support_layer_count() == 3);

}

TEST_CASE("SupportMaterial: Support generated for bridge_with_hole")
{
    slic3r_log->set_level(log_t::ALL);
    Model model {Model()};
    auto config {Slic3r::Config::new_from_defaults()};
    Test::TestMesh m { Test::TestMesh::overhang };
    config->set("support_material", true);

    std::cerr << "Set support_material\n";
    auto print {Test::init_print({m}, model, config, false)};

    print->objects.front()->_slice();

    CHECK(print->objects.front()->support_layer_count() > 0);

}

// Test 1.
SCENARIO("SupportMaterial: support_layers_z and contact_distance")
{
    GIVEN("A print object having one modelObject") {
        // Create a mesh & modelObject.
        TriangleMesh mesh = TriangleMesh::make_cube(20, 20, 20);

        // Create modelObject.
        Model model = Model();
        ModelObject *object = model.add_object();
        object->add_volume(mesh);
        model.add_default_instances();

        // Align to origin.
        model.align_instances_to_origin();
        // Create Print.
        Print print = Print();
        print.default_object_config.set_deserialize("support_material", "1");

        WHEN("First layer height = 0.4") {
            print.default_object_config.set_deserialize("layer_height", "0.2");
            print.default_object_config.set_deserialize("first_layer_height", "0.4");

            print.add_model_object(model.objects[0]);
            print.objects.front()->_slice();
            bool a, b, c, d;

            test_1_checks(print, a, b, c, d);
        }
        WHEN("Layer height = 0.2 and, first layer height = 0.3") {
            print.default_object_config.set_deserialize("layer_height", "0.2");
            print.default_object_config.set_deserialize("first_layer_height", "0.3");
            print.add_model_object(model.objects[0]);
            print.objects.front()->_slice();
            bool a, b, c, d;

            test_1_checks(print, a, b, c, d);
        }


        WHEN("Layer height = nozzle_diameter[0]") {
            print.default_object_config.set_deserialize("layer_height", "0.2");
            print.default_object_config.set_deserialize("first_layer_height", "0.3");
            print.add_model_object(model.objects[0]);
            print.objects.front()->_slice();
            bool a, b, c, d;

            test_1_checks(print, a, b, c, d);

        }
    }
}

// Test 8.
TEST_CASE("SupportMaterial: forced support is generated", "")
{
    // Create a mesh & modelObject.
    TriangleMesh mesh = TriangleMesh::make_cube(20, 20, 20);

    Model model = Model();
    ModelObject *object = model.add_object();
    object->add_volume(mesh);
    model.add_default_instances();
    model.align_instances_to_origin();

    Print print = Print();

    vector<coord_t> contact_z = {scale_(1.9)};
    vector<coord_t> top_z = {scale_(1.1)};
    print.default_object_config.support_material_enforce_layers = 100;
    print.default_object_config.support_material = 0;
    print.default_object_config.layer_height = 0.2;
    print.default_object_config.set_deserialize("first_layer_height", "0.3");

    print.add_model_object(model.objects[0]);
    print.objects.front()->_slice();

    SupportMaterial *support = print.objects.front()->_support_material();
    auto support_z = support->support_layers_z(contact_z, top_z, print.default_object_config.layer_height);

    bool check = true;
    for (size_t i = 1; i < support_z.size(); i++) {
        if (support_z[i] - support_z[i - 1] <= 0)
            check = false;
    }

    REQUIRE(check == true);
}

// Test 6.
SCENARIO("SupportMaterial: Checking bridge speed")
{
    GIVEN("Print object") {
        // Create a mesh & modelObject.
        TriangleMesh mesh = TriangleMesh::make_cube(20, 20, 20);

        Model model = Model();
        ModelObject *object = model.add_object();
        object->add_volume(mesh);
        model.add_default_instances();
        model.align_instances_to_origin();

        Print print = Print();
        print.config.brim_width = 0;
        print.config.skirts = 0;
        print.config.skirts = 0;
        print.default_object_config.support_material = 1;
        print.default_region_config.top_solid_layers = 0; // so that we don't have the internal bridge over infill.
        print.default_region_config.bridge_speed = 99;
        print.config.cooling = 0;
        print.config.set_deserialize("first_layer_speed", "100%");

        WHEN("support_material_contact_distance = 0.2") {
            print.default_object_config.support_material_contact_distance = 0.2;
            print.add_model_object(model.objects[0]);

            bool check = test_6_checks(print);
            REQUIRE(check == true); // bridge speed is used.
        }

        WHEN("support_material_contact_distance = 0") {
            print.default_object_config.support_material_contact_distance = 0;
            print.add_model_object(model.objects[0]);

            bool check = test_6_checks(print);
            REQUIRE(check == true); // bridge speed is not used.
        }

        WHEN("support_material_contact_distance = 0.2 & raft_layers = 5") {
            print.default_object_config.support_material_contact_distance = 0.2;
            print.default_object_config.raft_layers = 5;
            print.add_model_object(model.objects[0]);

            bool check = test_6_checks(print);
            REQUIRE(check == true); // bridge speed is used.
        }

        WHEN("support_material_contact_distance = 0 & raft_layers = 5") {
            print.default_object_config.support_material_contact_distance = 0;
            print.default_object_config.raft_layers = 5;
            print.add_model_object(model.objects[0]);

            bool check = test_6_checks(print);

            REQUIRE(check == true); // bridge speed is not used.
        }
    }
}

void test_1_checks(Print &print, bool &a, bool &b, bool &c, bool &d)
{
    vector<coord_t> contact_z = {scale_(1.9f)};
    vector<coord_t> top_z = {scale_(1.1f)};

    SupportMaterial *support = print.objects.front()->_support_material();

    vector<coordf_t>
        support_z = support->support_layers_z(contact_z, top_z, print.default_object_config.layer_height);
    
    THEN("First layer height is honored") {
        REQUIRE(support_z[0] == Approx(print.default_object_config.first_layer_height.value));
    }
    
    THEN("No null or negative support layers") {
        for (size_t i = 1; i < support_z.size(); ++i)
            CHECK(support_z[i] - support_z[i - 1] > 0);
    }

    THEN("No layers thicker than nozzle diameter") {
        for (size_t i = 1; i < support_z.size(); ++i)
            CHECK(support_z[i] - support_z[i - 1] <= Approx(print.config.nozzle_diameter.get_at(0) + EPSILON));

    }
    coordf_t expected_top_spacing = support
        ->contact_distance(print.default_object_config.layer_height,
                           print.config.nozzle_diameter.get_at(0));

    THEN("Layers above top surfaces are spaced correctly") {
        bool wrong_top_spacing = 0;
        for (coordf_t top_z_el : top_z) {
            // find layer index of this top surface.
            size_t layer_id = -1;
            for (size_t i = 0; i < support_z.size(); i++) {
                if (abs(support_z[i] - top_z_el) < EPSILON) {
                    layer_id = i;
                    i = support_z.size();
                }
            }
            std::cerr << "top z: " << support_z[layer_id] << " " << support_z[layer_id+1] << " " << support_z[layer_id+2] << " " 
                      << expected_top_spacing << "\n";

            // check that first support layer above this top surface (or the next one) is spaced with nozzle diameter
            CHECK(((support_z[layer_id + 1] - support_z[layer_id] == Approx(expected_top_spacing).margin(EPSILON)) || 
                   (support_z[layer_id + 2] - support_z[layer_id] == Approx(expected_top_spacing).margin(EPSILON))));
        }
    }
}

// TODO
bool test_6_checks(Print &print)
{
    bool has_bridge_speed = true;

    // Pre-Processing.
    PrintObject *print_object = print.objects.front();
    print_object->infill();
    SupportMaterial *support_material = print.objects.front()->_support_material();
    support_material->generate(print_object);
    // TODO but not needed in test 6 (make brims and make skirts).

    // Exporting gcode.
    // TODO validation found in Simple.pm


    return has_bridge_speed;
}
