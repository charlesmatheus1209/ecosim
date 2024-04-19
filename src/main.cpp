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
    bool vivo;
};

std::mutex m;
std::condition_variable cv_inicia_threads;
std::condition_variable cv_fim_threads;
std::vector<entity_t> entities;
std::vector<std::thread> vetor_de_threads;
int count_finished_threads = 0;
int entities_alive = 0;

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

pos_t find_herbivore_to_eat(entity_t e){
    std::vector<pos_t> possible_herbivore;

    for(int i = e.position.i - 1; i <= e.position.i + 1; i++){
        for(int j = e.position.j - 1; j <= e.position.j + 1; j++){
            if(i >= 0 && i < NUM_ROWS && j >= 0 && j < NUM_ROWS){
                // Não está na borda
                if(entity_grid[i][j].type == herbivore && entity_grid[i][j].vivo == true){
                    pos_t p;
                    p.i = i;
                    p.j = j;
                    possible_herbivore.push_back(p);
                }
            }
        }
    }

    static std::random_device rd;
    static std::mt19937 gen(rd());
    int x = possible_herbivore.size();
    if(possible_herbivore.size() > 0){
        std::uniform_real_distribution<> dis(0, possible_herbivore.size()-1);
        return possible_herbivore[dis(gen)];
    }else{
        return e.position;
    }

}

pos_t find_plant_to_eat(entity_t e){
    std::vector<pos_t> possible_plants;

    for(int i = e.position.i - 1; i <= e.position.i + 1; i++){
        for(int j = e.position.j - 1; j <= e.position.j + 1; j++){
            if(i >= 0 && i < NUM_ROWS && j >= 0 && j < NUM_ROWS){
                // Não está na borda
                if(entity_grid[i][j].type == plant && entity_grid[i][j].vivo == true){
                    pos_t p;
                    p.i = i;
                    p.j = j;
                    possible_plants.push_back(p);
                }
            }
        }
    }

    static std::random_device rd;
    static std::mt19937 gen(rd());
    int x = possible_plants.size();
    if(possible_plants.size() > 0){
        std::uniform_real_distribution<> dis(0, possible_plants.size()-1);
        return possible_plants[dis(gen)];
    }else{
        return e.position;
    }

}


pos_t find_new_position(entity_t e){
    pos_t new_pos;
    std::vector<pos_t> possible_positions;

    for(int i = e.position.i - 1; i <= e.position.i + 1; i++){
        for(int j = e.position.j - 1; j <= e.position.j + 1; j++){
            if(i >= 0 && i < NUM_ROWS && j >= 0 && j < NUM_ROWS){
                // Não está na borda
                if(entity_grid[i][j].type == empty && entity_grid[i][j].vivo != true){
                    pos_t p;
                    p.i = i;
                    p.j = j;
                    possible_positions.push_back(p);
                }
            }
        }
    }

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

double generate_random_number(double min, double max){
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(min, max);

    return dis(gen);
}



void plant_simulation(int i, int j){
    
   
    std::unique_lock lk(m);
    
    while(!run_threads){
        cv_inicia_threads.wait(lk);
    }

    pos_t my_plant;
    my_plant.i = i;
    my_plant.j = j;

    
    if(entity_grid[my_plant.i][my_plant.j].vivo == false){
        std::cout << "Planta i: " << i  << " j: " << j << "Morreu" << std::endl;
        return;
    }
    
    if(entity_grid[my_plant.i][my_plant.j].age >= PLANT_MAXIMUM_AGE){
        std::cout << "Planta i: " << i  << " j: " << j << "Morreu" << std::endl;
        entity_grid[my_plant.i][my_plant.j].type = empty;
        entity_grid[my_plant.i][my_plant.j].vivo = false;
        entity_grid[my_plant.i][my_plant.j].age = 0;
        entity_grid[my_plant.i][my_plant.j].energy = 0;
        return;
    }

    if(generate_random_number(0, 100) <= 100*PLANT_REPRODUCTION_PROBABILITY){
        std::cout << "Planta i: " << i  << " j: " << j << " procriando" << std::endl;
        
        pos_t new_pos = find_new_position(entity_grid[my_plant.i][my_plant.j]);

        if(!(new_pos.i == my_plant.i && new_pos.j == my_plant.j)){
            entity_grid[new_pos.i][new_pos.j].energy = 0;
            entity_grid[new_pos.i][new_pos.j].age = 0;
            entity_grid[new_pos.i][new_pos.j].position = new_pos;
            entity_grid[new_pos.i][new_pos.j].vivo = true;
            entity_grid[new_pos.i][new_pos.j].type = plant;
        }

        entity_grid[my_plant.i][my_plant.j].age++;
        return;
    }


    /*std::cout<< "Planta:\n Idade: " << entity_grid[my_plant.i][my_plant.j].age 
                << "linha: " << entity_grid[my_plant.i][my_plant.j].position.i
                << "Coluna: " << entity_grid[my_plant.i][my_plant.j].position.j 
                << "Vivo: " << entity_grid[my_plant.i][my_plant.j].vivo << std::endl;*/

    
    entity_grid[my_plant.i][my_plant.j].age++;
        
}

void herbivore_simulation(int i, int j){
    
    std::unique_lock lk(m);
    
    while(!run_threads){
        cv_inicia_threads.wait(lk);
    }

    pos_t my_herbivore;
    my_herbivore.i = i;
    my_herbivore.j = j;
    

    if(entity_grid[my_herbivore.i][my_herbivore.j].vivo == false){
        std::cout << "Herbivoro i: " << i  << " j: " << j << "Morreu" << std::endl;
        return;
    }

    if(entity_grid[my_herbivore.i][my_herbivore.j].age >= HERBIVORE_MAXIMUM_AGE || entity_grid[my_herbivore.i][my_herbivore.j].energy == 0){
        std::cout << "Herbivoro i: " << i  << " j: " << j << "Morreu" << std::endl;
        entity_grid[my_herbivore.i][my_herbivore.j].type = empty;
        entity_grid[my_herbivore.i][my_herbivore.j].vivo = false;
        entity_grid[my_herbivore.i][my_herbivore.j].age = 0;
        entity_grid[my_herbivore.i][my_herbivore.j].energy = 0;
        return;
    }


    if(generate_random_number(0,100) <= 100*HERBIVORE_EAT_PROBABILITY){ // Comer planta
        pos_t plant_to_eat = find_plant_to_eat(entity_grid[my_herbivore.i][my_herbivore.j]);
        if(!(plant_to_eat.i == my_herbivore.i && plant_to_eat.j == my_herbivore.j)){
            std::cout << "herbivoro i: " << i  << " j: " << j << "Comeu Planta" << std::endl;
            entity_grid[plant_to_eat.i][plant_to_eat.j].type = empty;
            entity_grid[plant_to_eat.i][plant_to_eat.j].vivo = false;
            entity_grid[plant_to_eat.i][plant_to_eat.j].age = 0;
            entity_grid[plant_to_eat.i][plant_to_eat.j].energy = 0;

            entity_grid[my_herbivore.i][my_herbivore.j].energy += 30;
            entity_grid[my_herbivore.i][my_herbivore.j].age ++;

            return;
        }
    }

    if(generate_random_number(0,100) <= 100*HERBIVORE_REPRODUCTION_PROBABILITY){ // Reprodução
        if(entity_grid[my_herbivore.i][my_herbivore.j].energy >= 20){
            pos_t new_pos = find_new_position(entity_grid[my_herbivore.i][my_herbivore.j]);

            if(!(new_pos.i == my_herbivore.i && new_pos.j == my_herbivore.j)){
                std::cout << "Herbivoro i: " << i  << " j: " << j << " Reproduziu" << std::endl;
                entity_grid[new_pos.i][new_pos.j].type = herbivore;
                entity_grid[new_pos.i][new_pos.j].vivo = true;
                entity_grid[new_pos.i][new_pos.j].age = 0;
                entity_grid[new_pos.i][new_pos.j].energy = 100;

                entity_grid[my_herbivore.i][my_herbivore.j].energy -= 10;
                entity_grid[my_herbivore.i][my_herbivore.j].age ++;
                return;
            }
        }
    }

    if(generate_random_number(0, 100) <= 100*HERBIVORE_MOVE_PROBABILITY){
        
        if(entity_grid[my_herbivore.i][my_herbivore.j].energy >= 5){
            pos_t new_pos = find_new_position(entity_grid[my_herbivore.i][my_herbivore.j]);
            if(!(new_pos.i == my_herbivore.i && new_pos.j == my_herbivore.j)){
                std::cout << "Herbivoro i: " << i << " j: " << j << " Mudando de lugar" << std::endl;
                entity_grid[new_pos.i][new_pos.j] = entity_grid[my_herbivore.i][my_herbivore.j];
                entity_grid[new_pos.i][new_pos.j].energy -= 5;
                entity_grid[new_pos.i][new_pos.j].age += 1;
                entity_grid[new_pos.i][new_pos.j].position = new_pos;

                entity_grid[my_herbivore.i][my_herbivore.j].type = empty;
                entity_grid[my_herbivore.i][my_herbivore.j].vivo = false;
                entity_grid[my_herbivore.i][my_herbivore.j].age = 0;
                entity_grid[my_herbivore.i][my_herbivore.j].energy = 0;
            }else{
              entity_grid[my_herbivore.i][my_herbivore.j].age ++;  
            }

            return;
        }
    }



   /* std::cout<< "Herbivoro:\n Idade: " << entity_grid[my_herbivore.i][my_herbivore.j].age 
                << "linha: " << entity_grid[my_herbivore.i][my_herbivore.j].position.i
                << "Coluna: " << entity_grid[my_herbivore.i][my_herbivore.j].position.j
                << "Vivo: " << entity_grid[my_herbivore.i][my_herbivore.j].vivo << std::endl;*/

    
    entity_grid[my_herbivore.i][my_herbivore.j].age++;          
        
}

void carnivore_simulation(int i, int j){
   
    std::unique_lock lk(m);
    
    while(!run_threads){
        cv_inicia_threads.wait(lk);
    }

    pos_t my_carnivore;
    my_carnivore.i = i;
    my_carnivore.j = j;
    

    if(entity_grid[my_carnivore.i][my_carnivore.j].vivo == false){
        std::cout << "Carnivoro i: " << i << " j: " << j << " Morreu" << std::endl;
        entity_grid[my_carnivore.i][my_carnivore.j].age++;
        return;
    }

    if(entity_grid[my_carnivore.i][my_carnivore.j].age >= CARNIVORE_MAXIMUM_AGE || entity_grid[my_carnivore.i][my_carnivore.j].energy == 0){
        std::cout << "Carnivoro i: " << i  << " j: " << j << "Morreu" << std::endl;
        entity_grid[my_carnivore.i][my_carnivore.j].type = empty;
        entity_grid[my_carnivore.i][my_carnivore.j].vivo = false;
        entity_grid[my_carnivore.i][my_carnivore.j].age = 0;
        entity_grid[my_carnivore.i][my_carnivore.j].energy = 0;
        entity_grid[my_carnivore.i][my_carnivore.j].age++;
        return;
    }
    int x = generate_random_number(0,100);
    if(x <= 100*CARNIVORE_EAT_PROBABILITY){ // Comer herbivoro
        pos_t herbivore_to_eat = find_herbivore_to_eat(entity_grid[my_carnivore.i][my_carnivore.j]);

        if(!(herbivore_to_eat.i == my_carnivore.i && herbivore_to_eat.j == my_carnivore.j)){
            std::cout << "Carnivoro i: " << i << " j: " << j << " Comeu Herbivoro" << std::endl;
            entity_grid[herbivore_to_eat.i][herbivore_to_eat.j].type = empty;
            entity_grid[herbivore_to_eat.i][herbivore_to_eat.j].vivo = false;
            entity_grid[herbivore_to_eat.i][herbivore_to_eat.j].age = 0;
            entity_grid[herbivore_to_eat.i][herbivore_to_eat.j].energy = 0;

            entity_grid[my_carnivore.i][my_carnivore.j].energy += 20;
            entity_grid[my_carnivore.i][my_carnivore.j].age++;

            return;
        }
    }

    if(generate_random_number(0,100) <= 100*CARNIVORE_REPRODUCTION_PROBABILITY){ // Reprodução
        if(entity_grid[my_carnivore.i][my_carnivore.j].energy >= 20){
            pos_t new_pos = find_new_position(entity_grid[my_carnivore.i][my_carnivore.j]);

            if(!(new_pos.i == my_carnivore.i && new_pos.j == my_carnivore.j)){
                std::cout << "Carnivoro i: " << i << " j: " << j << " Reproduziu" << std::endl;
                entity_grid[new_pos.i][new_pos.j].type = carnivore;
                entity_grid[new_pos.i][new_pos.j].vivo = true;
                entity_grid[new_pos.i][new_pos.j].age = 0;
                entity_grid[new_pos.i][new_pos.j].energy = 100;

                entity_grid[my_carnivore.i][my_carnivore.j].energy -= 10;
                entity_grid[my_carnivore.i][my_carnivore.j].age ++;
                return;
            }
        }
    }

    if(generate_random_number(0, 100) <= 100*CARNIVORE_MOVE_PROBABILITY){
        if(entity_grid[my_carnivore.i][my_carnivore.j].energy >= 5){
            pos_t new_pos = find_new_position(entity_grid[my_carnivore.i][my_carnivore.j]);

            if(!(new_pos.i == my_carnivore.i && new_pos.j == my_carnivore.j)){
                std::cout << "Carnivore i: " << i << " j : " << j << " Mudando de lugar" << std::endl;
                entity_grid[new_pos.i][new_pos.j] = entity_grid[my_carnivore.i][my_carnivore.j];
                entity_grid[new_pos.i][new_pos.j].energy -= 5;
                entity_grid[new_pos.i][new_pos.j].age += 1;
                entity_grid[new_pos.i][new_pos.j].position = new_pos;

                entity_grid[my_carnivore.i][my_carnivore.j].type = empty;
                entity_grid[my_carnivore.i][my_carnivore.j].vivo = false;
                entity_grid[my_carnivore.i][my_carnivore.j].age = 0;
                entity_grid[my_carnivore.i][my_carnivore.j].energy = 0;
            }else{
                entity_grid[my_carnivore.i][my_carnivore.j].age ++;
            }
            return;
        }
    }

        
   /*std::cout<< "Carnivoro:\n Idade: " << entity_grid[my_carnivore.i][my_carnivore.j].age 
                << "linha: " << entity_grid[my_carnivore.i][my_carnivore.j].position.i
                << "Coluna: " << entity_grid[my_carnivore.i][my_carnivore.j].position.j
                << "Vivo: " << entity_grid[my_carnivore.i][my_carnivore.j].vivo << std::endl;*/
    
    entity_grid[my_carnivore.i][my_carnivore.j].age++;
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
        entities.clear();
        
        // Create the entities
        // <YOUR CODE HERE>

        m.lock();
        
        for(int i = 0; i < (uint32_t)request_body["plants"]; i++){
            entity_t _plant;
            
            _plant.age = 0;
            _plant.energy = 0;
            _plant.type = plant;
            _plant.vivo = true;

            entities.push_back(_plant);
        }

        for(int i = 0; i < (uint32_t)request_body["herbivores"]; i++){
            entity_t _herb;
            
            _herb.age = 0;
            _herb.energy = 100;
            _herb.type = herbivore;
            _herb.vivo = true;

            entities.push_back(_herb);
        }

        for(int i = 0; i < (uint32_t)request_body["carnivores"]; i++){
            entity_t _carni;
            
            _carni.age = 0;
            _carni.energy = 100;
            _carni.type = carnivore;
            _carni.vivo = true;

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

            entity_grid[row][col] = entities[i];
        }
        m.unlock();

           

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
            vetor_de_threads.clear();
        
            for(int i = 0; i < entity_grid.size(); i++){
                for(int j = 0; j < entity_grid[i].size(); j++){
                    if(entity_grid[i][j].vivo == true & entity_grid[i][j].type != empty){
                        if(entity_grid[i][j].type == carnivore){
                            vetor_de_threads.push_back(std::thread(carnivore_simulation, i, j));
                        }else if(entity_grid[i][j].type == herbivore){
                            vetor_de_threads.push_back(std::thread(herbivore_simulation, i, j));
                        }else if(entity_grid[i][j].type == plant){
                            vetor_de_threads.push_back(std::thread(plant_simulation, i, j));
                        }
                    }
                }
            }

            run_threads = true;
        m.unlock();

        cv_inicia_threads.notify_all();

        for(int i = 0; i < vetor_de_threads.size(); i++){
            vetor_de_threads[i].join();
        }  
        


        m.lock();
        run_threads = false;
        m.unlock();


        // Return the JSON representation of the entity grid
        nlohmann::json json_grid = entity_grid; 
        return json_grid.dump(); });
    app.port(8080).run();

    
    return 0;
}