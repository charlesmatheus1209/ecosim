#define CROW_MAIN
#define CROW_STATIC_DIR "../public"

#include "crow_all.h"
#include "json.hpp"
#include <random>

static const uint32_t NUM_ROWS = 15;

// Constants
const uint32_t PLANT_MAXIMUM_AGE = 10;
const uint32_t HERBIVORE_MAXIMUM_AGE = 50;
const uint32_t CARNIVORE_MAXIMUM_AGE = 80;
const uint32_t MAXIMUM_ENERGY = 200;
const uint32_t THRESHOLD_ENERGY_FOR_REPRODUCTION = 20;

// Probabilities
const double PLANT_REPRODUCTION_PROBABILITY = 0.2;
const double HERBIVORE_REPRODUCTION_PROBABILITY = 0.075;
const double CARNIVORE_REPRODUCTION_PROBABILITY = 0.025;
const double HERBIVORE_MOVE_PROBABILITY = 0.7;
const double HERBIVORE_EAT_PROBABILITY = 0.9;
const double CARNIVORE_MOVE_PROBABILITY = 0.5;
const double CARNIVORE_EAT_PROBABILITY = 1.0;


std::mutex m;
std::condition_variable cv_grid;

// Type definitions
enum entity_type_t
{
    empty,
    plant,
    herbivore,
    carnivore
};

struct pos_t
{
    uint32_t i;
    uint32_t j;
};

struct entity_t
{
    entity_type_t type;
    int32_t energy;
    int32_t age;
    int32_t row;
    int32_t col;
};

// Auxiliary code to convert the entity_type_t enum to a string
NLOHMANN_JSON_SERIALIZE_ENUM(entity_type_t, {
                                                {empty, " "},
                                                {plant, "P"},
                                                {herbivore, "H"},
                                                {carnivore, "C"},
                                            })

// Auxiliary code to convert the entity_t struct to a JSON object
namespace nlohmann
{
    void to_json(nlohmann::json &j, const entity_t &e)
    {
        j = nlohmann::json{{"type", e.type}, {"energy", e.energy}, {"age", e.age}};
    }
}




void plant_simulation(){
    while(true){
        // std::unique_lock lk(m);
        // cv_grid.wait(lk, []{ return 0; });
    }
}

void herbivore_simulation(){
    while(true){
        // std::unique_lock lk(m);
        // cv_grid.wait(lk, []{ return 0; });
    }
}

void carnivore_simulation(){
    while(true){
        // std::unique_lock lk(m);
        // cv_grid.wait(lk, []{ return 0; });
    }
}



// Grid that contains the entities
static std::vector<std::vector<entity_t>> entity_grid;

int main()
{
    crow::SimpleApp app;

    // Endpoint to serve the HTML page
    CROW_ROUTE(app, "/")
    ([](crow::request &, crow::response &res)
     {
        // Return the HTML content here
        res.set_static_file_info_unsafe("../public/index.html");
        res.end(); });

    CROW_ROUTE(app, "/start-simulation")
        .methods("POST"_method)([](crow::request &req, crow::response &res)
                                { 
        // Parse the JSON request body
        nlohmann::json request_body = nlohmann::json::parse(req.body);

       // Validate the request body 
        uint32_t total_entinties = (uint32_t)request_body["plants"] + (uint32_t)request_body["herbivores"] + (uint32_t)request_body["carnivores"];
        if (total_entinties > NUM_ROWS * NUM_ROWS) {
        res.code = 400;
        res.body = "Too many entities";
        res.end();
        return;
        }

        // Clear the entity grid
        entity_grid.clear();
        entity_grid.assign(NUM_ROWS, std::vector<entity_t>(NUM_ROWS, { empty, 0, 0}));
        
        // Create the entities
        // <YOUR CODE HERE>

        std::vector<entity_t> entities;
        for(int i = 0; i < (uint32_t)request_body["plants"]; i++){
            entity_t _plant;
            
            _plant.age = 0;
            _plant.energy = 0;
            _plant.type = plant;

            entities.push_back(_plant);
        }

        for(int i = 0; i < (uint32_t)request_body["herbivores"]; i++){
            entity_t _herb;
            
            _herb.age = 0;
            _herb.energy = 100;
            _herb.type = herbivore;

            entities.push_back(_herb);
        }

         for(int i = 0; i < (uint32_t)request_body["carnivores"]; i++){
            entity_t _carni;
            
            _carni.age = 0;
            _carni.energy = 100;
            _carni.type = carnivore;

            entities.push_back(_carni);
        }

        for(int i = 0; i < entities.size(); i++){
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis(0, 15);


            std::cout << "entidade: " << i << std::endl;
            int row = dis(gen);
            int col = dis(gen);

            while(entity_grid[row][col].type != empty){
                
                std::cout << "ROW: " << row << "COL" << col << std::endl;
                row = dis(gen);
                col = dis(gen);
            }

            entities[i].row = row;
            entities[i].col = col;

            if(entities[i].type == carnivore){
                
            }else if(entities[i].type == herbivore){

            }else if(entities[i].type == plant){

            }
            entity_grid[row][col] = entities[i];

        }
        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        res.body = json_grid.dump();
        res.end(); });

    // Endpoint to process HTTP GET requests for the next simulation iteration
    CROW_ROUTE(app, "/next-iteration")
        .methods("GET"_method)([]()
                               {
        // Simulate the next iteration
        // Iterate over the entity grid and simulate the behaviour of each entity
        
        // <YOUR CODE HERE>
        
        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        return json_grid.dump(); });
    app.port(8080).run();

    return 0;
}