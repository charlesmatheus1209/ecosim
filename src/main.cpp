#define CROW_MAIN
#define CROW_STATIC_DIR "../public"

#include "crow_all.h"
#include "json.hpp"
#include <random>


static const uint32_t NUM_ROWS = 15;

// Constants
const uint32_t PLANT_MAXIMUM_AGE = 10;
//const uint32_t HERBIVORE_MAXIMUM_AGE = 50;
const uint32_t HERBIVORE_MAXIMUM_AGE = 10;
//const uint32_t CARNIVORE_MAXIMUM_AGE = 80;
const uint32_t CARNIVORE_MAXIMUM_AGE = 20;
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
    int32_t i;
    int32_t j;
};

struct entity_t
{
    entity_type_t type;
    int32_t energy;
    int32_t age;
    pos_t position;
    bool ja_rodei;
    int32_t id;
};

std::mutex m;
std::condition_variable cv_grid;
std::vector<entity_t> entities;
std::vector<std::thread> vetor_de_threads;
int count_finished_threads = 0;
int last_id = 0;

bool run_threads = false;

// Grid that contains the entities
static std::vector<std::vector<entity_t>> entity_grid;

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

pos_t find_new_position(entity_t e){
    pos_t new_pos;
    std::vector<pos_t> possible_positions;

    for(int i = e.position.i - 1; i <= e.position.i + 1; i++){
        for(int j = e.position.j - 1; j <= e.position.j + 1; j++){
            //std::cout << "i = " << i << " j = " << j << std::endl;
            if(i >= 0 && i < NUM_ROWS && j >= 0 && j < NUM_ROWS){
                // Não está na borda
                if(entity_grid[i][j].type == empty){
                    pos_t p;
                    p.i = i;
                    p.j = j;
                    possible_positions.push_back(p);
                }
            }
        }
    }

    /*for(int i = 0; i < possible_positions.size(); i++){
        std::cout << "i = " << possible_positions[i].i << " j = " << possible_positions[i].j << std::endl;
    }*/

    static std::random_device rd;
    static std::mt19937 gen(rd());
    int x = possible_positions.size();
    if(possible_positions.size() > 0){
        std::uniform_real_distribution<> dis(0, possible_positions.size()-1);
        return possible_positions[dis(gen)];
    }else{
        return e.position;
    }
}

void atualiza_grid(){
    entity_grid.clear();
    entity_grid.assign(NUM_ROWS, std::vector<entity_t>(NUM_ROWS, { empty, 0, 0}));
    for(int i = 0; i < entities.size(); i++){
        int row = entities[i].position.i;
        int col = entities[i].position.j;
        if(row >=0 && col >= 0 && entities[i].type != empty){
            entity_grid[row][col] = entities[i];
        }
    }
}

double generate_random_number(double min, double max){
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(min, max);

    return dis(gen);
}

void kill_entity(entity_t e){
    int to_remove = -1;

    for(int i = 0; i < entities.size(); i++){
        if(entities[i].id == e.id){
            to_remove = i;
        }
    }

    if(to_remove >= 0){
        entities.erase(entities.begin() + to_remove);
    }
}

void plant_simulation(entity_t* my_plant){
    
    while(true){
        {
            std::unique_lock lk(m);
            
            while(!run_threads || my_plant->ja_rodei){
                cv_grid.wait(lk);
            }

            /*std::cout<< "Planta:\n Idade: " << my_plant->age 
                     << "linha: " << my_plant->position.i
                     << "Coluna: " << my_plant->position.j << std::endl;*/

           
            my_plant->age++;

            count_finished_threads++;
            my_plant->ja_rodei = true;
        }
    }
}

void herbivore_simulation(entity_t* my_herbivore){
    while(true){
        {
            std::unique_lock lk(m);
            
            while(!run_threads || my_herbivore->ja_rodei){
                cv_grid.wait(lk);
            }

            /*std::cout<< "Herbivoro:\n Idade: " << my_herbivore->age 
                     << "linha: " << my_herbivore->position.i
                     << "Coluna: " << my_herbivore->position.j<< std::endl;*/

            if(my_herbivore->age <= HERBIVORE_MAXIMUM_AGE){
                my_herbivore->age++;
                
                if(generate_random_number(0, 100) > 100*HERBIVORE_MOVE_PROBABILITY){
                    if(my_herbivore->energy >= 5){
                        pos_t new_pos = find_new_position(*my_herbivore);   
                        my_herbivore->position = new_pos;
                        my_herbivore->energy -= 5;
                    }                
                }
                
            }else{
                kill_entity(*my_herbivore);
                std::cout << "Removidooooooo" << std::endl;
                count_finished_threads++;
                break;
            }
            
            count_finished_threads++;
            my_herbivore->ja_rodei = true;
        }
    }
}

void carnivore_simulation(entity_t* my_carnivore){
    while(true){
        {
            std::unique_lock lk(m);
            
            while(!run_threads || my_carnivore->ja_rodei){
                cv_grid.wait(lk);
            }
            
            /*std::cout<< "Carnivoro:\n Idade: " << my_carnivore->age 
                    << "linha: " << my_carnivore->position.i
                    << "Coluna: " << my_carnivore->position.j << std::endl;*/

            if(my_carnivore->age <= CARNIVORE_MAXIMUM_AGE){
                if(generate_random_number(0, 100) > 100*CARNIVORE_MOVE_PROBABILITY){
                    if(my_carnivore->energy >= 5){
                        pos_t new_pos = find_new_position(*my_carnivore);    
                        my_carnivore->position = new_pos;
                        my_carnivore->energy -= 5;
                    }                
                }
                
                
                my_carnivore->age++;
            }else{
                /*kill_entity(*my_carnivore);

                std::cout << "Removidooooooo" << std::endl;
                count_finished_threads++;
                m.unlock();
                return;*/
            }
            
            count_finished_threads++;            
            my_carnivore->ja_rodei = true;
        }
    }
}



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

        
        for(int i = 0; i < (uint32_t)request_body["plants"]; i++){
            entity_t _plant;
            
            _plant.age = 0;
            _plant.energy = 0;
            _plant.type = plant;
            _plant.ja_rodei = false;

            entities.push_back(_plant);
        }

        for(int i = 0; i < (uint32_t)request_body["herbivores"]; i++){
            entity_t _herb;
            
            _herb.age = 0;
            _herb.energy = 100;
            _herb.type = herbivore;
            _herb.ja_rodei = false;

            entities.push_back(_herb);
        }

        for(int i = 0; i < (uint32_t)request_body["carnivores"]; i++){
            entity_t _carni;
            
            _carni.age = 0;
            _carni.energy = 100;
            _carni.type = carnivore;
            _carni.ja_rodei = false;

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

            entities[i].position.i = row;
            entities[i].position.j = col;
            entities[i].id = last_id++;

            entity_grid[row][col] = entities[i];

        }

        for(int i = 0; i < entities.size(); i++){
            if(entities[i].type == carnivore){
                vetor_de_threads.push_back(std::thread(carnivore_simulation, &entities[i]));
            }else if(entities[i].type == herbivore){
                vetor_de_threads.push_back(std::thread(herbivore_simulation, &entities[i]));
            }else if(entities[i].type == plant){
                vetor_de_threads.push_back(std::thread(plant_simulation, &entities[i]));
            }
        }

        for(int i = 0; i < vetor_de_threads.size(); i++){
            vetor_de_threads[i].detach();
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

        m.lock();
        count_finished_threads = 0;
        run_threads = true;

        for(int i = 0; i < entities.size(); i++){
            entities[i].ja_rodei = false;
        }
        m.unlock();

        cv_grid.notify_all();
        
        int www = 0;
        while(count_finished_threads < entities.size()){
            www = entities.size();
        }
        ;

        m.lock();
        run_threads = false;

        atualiza_grid();
        m.unlock();

        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        return json_grid.dump(); });
    app.port(8080).run();

    
    return 0;
}