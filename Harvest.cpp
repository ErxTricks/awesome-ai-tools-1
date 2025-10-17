#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <random>
#include <algorithm>
#include <limits>

// --- Generator Angka Acak ---
std::mt19_381 rng(std::chrono::steady_clock::now().time_since_epoch().count());

// --- Struktur Data Inti ---

enum class Skill { Farming, Mining, Foraging, Combat };
enum class Location { Farm, Village, Forest, Mines };

struct Item {
    std::string name;
    std::string type; // RESOURCE, TOOL, SEED, CROP, WEAPON
    int value;
};

struct Player {
    std::string name;
    int day = 1;
    int energy;
    int maxEnergy = 100;
    int gold = 100;

    std::map<Skill, int> skillLevels;
    std::map<Skill, int> skillXp;
    std::map<std::string, int> inventory;
    std::map<std::string, int> tools; // Tool Name -> Level

    Location currentLocation = Location::Farm;
};

struct CraftingRecipe {
    std::string resultItem;
    std::map<std::string, int> ingredients;
};

// --- Database & Variabel Global ---
Player player;
bool isGameRunning = true;
std::map<std::string, Item> itemDB;
std::map<std::string, CraftingRecipe> recipeDB;
bool bridgeRepaired = false;

// --- Fungsi Bantu ---
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}
void separator() { std::cout << "\n--------------------------------------------------\n"; }
void shortPause(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
void pause() {
    std::cout << "\n(Tekan Enter untuk melanjutkan...)";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
}

// --- Inisialisasi Game ---
void initializeDatabase() {
    // Items
    itemDB["Kayu"] = {"Kayu", "RESOURCE", 2};
    itemDB["Batu"] = {"Batu", "RESOURCE", 3};
    itemDB["Tembaga"] = {"Biji Tembaga", "RESOURCE", 10};
    itemDB["Besi"] = {"Biji Besi", "RESOURCE", 25};
    itemDB["Benih Lobak"] = {"Benih Lobak", "SEED", 10};
    itemDB["Lobak"] = {"Lobak", "CROP", 30};
    itemDB["Slime"] = {"Gumpalan Slime", "RESOURCE", 5};

    // Tools
    itemDB["Cangkul"] = {"Cangkul", "TOOL", 0};
    itemDB["Beliung"] = {"Beliung", "TOOL", 0};
    itemDB["Pedang"] = {"Pedang", "WEAPON", 0};
    
    // Recipes
    recipeDB["Beliung Tembaga"] = {"Beliung Tembaga", {{"Beliung", 1}, {"Biji Tembaga", 10}, {"Kayu", 20}}};
    recipeDB["Pedang Besi"] = {"Pedang Besi", {{"Pedang", 1}, {"Biji Besi", 15}, {"Kayu", 10}}};
}

// --- Sistem Inti ---

void gainXp(Skill s, int amount) {
    player.skillXp[s] += amount;
    std::cout << "+ " << amount << " XP " << (s == Skill::Farming ? "Bertani" : s == Skill::Mining ? "Menambang" : s == Skill::Foraging ? "Mencari Hasil Hutan" : "Bertarung") << "!\n";
    
    int xpToNext = 100 * player.skillLevels[s];
    if (player.skillXp[s] >= xpToNext) {
        player.skillLevels[s]++;
        player.skillXp[s] -= xpToNext;
        std::cout << "\n*** KETERAMPILAN MENINGKAT! ***\n";
        std::cout << (s == Skill::Farming ? "Bertani" : s == Skill::Mining ? "Menambang" : s == Skill::Foraging ? "Mencari Hasil Hutan" : "Bertarung") << " sekarang Level " << player.skillLevels[s] << "!\n";
        pause();
    }
}

bool consumeEnergy(int amount) {
    if (player.energy >= amount) {
        player.energy -= amount;
        return true;
    }
    std::cout << "Energi tidak cukup! Kamu terlalu lelah.\n";
    shortPause(1500);
    return false;
}

void displayHUD() {
    std::cout << "Hari: " << player.day << " | Lokasi: " 
              << (player.currentLocation == Location::Farm ? "Ladang" : player.currentLocation == Location::Village ? "Desa" : player.currentLocation == Location::Forest ? "Hutan" : "Tambang")
              << " | Energi: " << player.energy << "/" << player.maxEnergy << " | Emas: " << player.gold << " G\n";
    separator();
}

void goToSleep() {
    clearScreen();
    std::cout << "Kamu pergi tidur untuk mengakhiri hari...\n";
    player.day++;
    player.energy = player.maxEnergy;
    std::cout << "Selamat pagi! Hari ke-" << player.day << " telah dimulai.\n";
    pause();
}

// --- Aktivitas ---

void doForaging() {
    if (!consumeEnergy(10)) return;

    std::cout << "Kamu mencari hasil hutan di sekitar...\n";
    shortPause(1500);
    
    std::uniform_int_distribution<int> dist(1, 100);
    int roll = dist(rng);

    if (roll > 50) {
        std::cout << "Kamu menemukan beberapa Kayu!\n";
        player.inventory["Kayu"] += 3;
        gainXp(Skill::Foraging, 5);
    } else if (roll > 20) {
        std::cout << "Kamu menemukan beberapa Batu!\n";
        player.inventory["Batu"] += 2;
        gainXp(Skill::Foraging, 3);
    } else {
        std::cout << "Kamu tidak menemukan apapun yang berguna.\n";
    }
}

void doMining() {
    if (player.tools["Beliung"] == 0) {
        std::cout << "Kamu tidak punya Beliung untuk menambang!\n";
        return;
    }
    if (!consumeEnergy(15)) return;

    std::cout << "Kamu mengayunkan beliung ke bebatuan...\n";
    shortPause(2000);

    std::uniform_int_distribution<int> dist(1, 100);
    int roll = dist(rng) + (player.tools["Beliung"] * 5); // Beliung lebih baik meningkatkan peluang

    if (roll > 85 && player.skillLevels[Skill::Mining] >= 3) {
        std::cout << "Luar biasa! Kamu menemukan Biji Besi!\n";
        player.inventory["Biji Besi"]++;
        gainXp(Skill::Mining, 25);
    } else if (roll > 60) {
        std::cout << "Kamu menemukan Biji Tembaga!\n";
        player.inventory["Biji Tembaga"]++;
        gainXp(Skill::Mining, 10);
    } else if (roll > 20) {
        std::cout << "Kamu mendapatkan beberapa Batu.\n";
        player.inventory["Batu"] += 3;
        gainXp(Skill::Mining, 5);
    } else {
        std::cout << "Batu itu hancur menjadi debu.\n";
    }
}

void startCombat() {
    if (player.tools["Pedang"] == 0) {
        std::cout << "Terlalu berbahaya masuk lebih dalam tanpa pedang!\n";
        return;
    }
    if (!consumeEnergy(20)) return;

    std::cout << "Kamu masuk lebih dalam dan bertemu Slime!\n";
    int playerHP = 100;
    int enemyHP = 30 + (player.skillLevels[Skill::Combat] * 5);

    while(playerHP > 0 && enemyHP > 0) {
        // Player turn
        int playerDamage = 10 + (player.tools["Pedang"] * 5) + player.skillLevels[Skill::Combat];
        std::cout << "Kamu menyerang Slime dan memberikan " << playerDamage << " kerusakan!\n";
        enemyHP -= playerDamage;
        if (enemyHP <= 0) break;
        
        // Enemy turn
        int enemyDamage = 5 + player.day; // Semakin hari, musuh makin kuat
        std::cout << "Slime menyerangmu dan memberikan " << enemyDamage << " kerusakan!\n";
        playerHP -= enemyDamage;
        shortPause(1000);
    }

    if (playerHP > 0) {
        std::cout << "Kamu mengalahkan Slime!\n";
        player.inventory["Gumpalan Slime"] += 2;
        gainXp(Skill::Combat, 15);
    } else {
        std::cout << "Kamu kalah! Kamu pingsan dan terbangun di pintu masuk tambang...\n";
        player.energy = 10;
    }
}

void visitShop() {
    clearScreen();
    std::cout << "--- TOKO DESA ---\n";
    std::cout << "Emas-mu: " << player.gold << " G\n";
    separator();
    std::cout << "[1] Beli Benih Lobak (10 G)\n";
    std::cout << "[2] Jual Barang\n";
    std::cout << "[3] Kembali\n> ";
    std::string choice;
    std::getline(std::cin, choice);

    if (choice == "1") {
        if (player.gold >= 10) {
            player.gold -= 10;
            player.inventory["Benih Lobak"]++;
            std::cout << "Kamu membeli 1 Benih Lobak.\n";
        } else {
            std::cout << "Emas tidak cukup!\n";
        }
    } else if (choice == "2") {
        std::cout << "Barang apa yang ingin kamu jual? (Contoh: Kayu, Lobak)\n> ";
        std::getline(std::cin, choice);
        if (player.inventory[choice] > 0 && itemDB.count(choice)) {
            int qty = player.inventory[choice];
            int price = itemDB[choice].value;
            player.inventory.erase(choice);
            player.gold += qty * price;
            std::cout << "Kamu menjual " << qty << " " << choice << " seharga " << qty * price << " G.\n";
        } else {
            std::cout << "Kamu tidak punya barang itu untuk dijual.\n";
        }
    }
    pause();
}

void doCrafting() {
    clearScreen();
    std::cout << "--- MEJA KERAJINAN ---\n";
    std::cout << "Pilih resep untuk dibuat:\n";
    std::cout << "[1] Beliung Tembaga (Butuh: 1 Beliung, 10 Biji Tembaga, 20 Kayu)\n";
    std::cout << "[2] Pedang Besi (Butuh: 1 Pedang, 15 Biji Besi, 10 Kayu)\n";
    std::cout << "[3] Perbaiki Jembatan (Butuh: 150 Kayu, 150 Batu, 50 Biji Besi) - TUJUAN UTAMA\n";
    std::cout << "[4] Kembali\n> ";

    std::string choice;
    std::getline(std::cin, choice);
    
    if (choice == "3") {
        if (player.inventory["Kayu"] >= 150 && player.inventory["Batu"] >= 150 && player.inventory["Biji Besi"] >= 50) {
            player.inventory["Kayu"] -= 150;
            player.inventory["Batu"] -= 150;
            player.inventory["Biji Besi"] -= 50;
            bridgeRepaired = true;
            std::cout << "Kamu bekerja keras sepanjang hari dan berhasil memperbaiki jembatan!\n";
        } else {
            std::cout << "Bahan tidak cukup untuk memperbaiki jembatan!\n";
        }
    }
    // Implementasi crafting lain bisa ditambahkan di sini
    pause();
}


// --- Loop Lokasi ---
void atTheFarm() {
    std::cout << "Kamu berada di ladangmu. Apa yang akan kamu lakukan?\n";
    std::cout << "[1] Pergi ke Desa\n";
    std::cout << "[2] Pergi ke Hutan\n";
    std::cout << "[3] Pergi ke Tambang\n";
    std::cout << "[4] Buka Meja Kerajinan\n";
    std::cout << "[5] Lihat Inventaris & Status\n";
    std::cout << "[6] Tidur (Akhiri Hari)\n> ";
    std::string choice;
    std::getline(std::cin, choice);

    if (choice == "1") player.currentLocation = Location::Village;
    else if (choice == "2") player.currentLocation = Location::Forest;
    else if (choice == "3") player.currentLocation = Location::Mines;
    else if (choice == "4") doCrafting();
    // else if (choice == "5") displayStatus(); // Fungsi ini bisa dibuat untuk detail
    else if (choice == "6") goToSleep();
}

void inTheVillage() {
    std::cout << "Kamu berada di desa yang ramai.\n";
    std::cout << "[1] Kunjungi Toko\n";
    std::cout << "[2] Kembali ke Ladang\n> ";
    std::string choice;
    std::getline(std::cin, choice);

    if (choice == "1") visitShop();
    else if (choice == "2") player.currentLocation = Location::Farm;
}

void inTheForest() {
    std::cout << "Kamu berada di Hutan Belantara.\n";
    std::cout << "[1] Mencari Hasil Hutan\n";
    std::cout << "[2] Kembali ke Ladang\n> ";
    std::string choice;
    std::getline(std::cin, choice);
    if (choice == "1") doForaging();
    else if (choice == "2") player.currentLocation = Location::Farm;
}

void inTheMines() {
    std::cout << "Kamu berada di pintu masuk tambang yang gelap.\n";
    std::cout << "[1] Menambang di Dekat Pintu Masuk\n";
    std::cout << "[2] Masuk Lebih Dalam (Bertarung)\n";
    std::cout << "[3] Kembali ke Ladang\n> ";
    std::string choice;
    std::getline(std::cin, choice);
    if (choice == "1") doMining();
    else if (choice == "2") startCombat();
    else if (choice == "3") player.currentLocation = Location::Farm;
}

// --- Main Function ---
int main() {
    initializeDatabase();
    
    // Inisialisasi Player
    player.energy = player.maxEnergy;
    player.skillLevels = {{Skill::Farming, 1}, {Skill::Mining, 1}, {Skill::Foraging, 1}, {Skill::Combat, 1}};
    player.tools["Cangkul"] = 1;
    player.tools["Beliung"] = 1;
    player.tools["Pedang"] = 1;

    clearScreen();
    std::cout << "=======================================\n";
    std::cout << "||     KRONIK LEMBAH HARAPAN         ||\n";
    std::cout << "=======================================\n";
    std::cout << "Siapakah nama petualang kita?\n> ";
    std::getline(std::cin, player.name);

    while (isGameRunning) {
        clearScreen();
        displayHUD();

        switch (player.currentLocation) {
            case Location::Farm: atTheFarm(); break;
            case Location::Village: inTheVillage(); break;
            case Location::Forest: inTheForest(); break;
            case Location::Mines: inTheMines(); break;
        }

        if (bridgeRepaired) {
            clearScreen();
            std::cout << "*****************************************************\n";
            std::cout << "                 SELAMAT, " << player.name << "!\n";
            std::cout << "*****************************************************\n";
            std::cout << "Dengan kerja kerasmu, Jembatan Desa telah diperbaiki!\n";
            std::cout << "Lembah Harapan kini kembali terhubung dengan dunia luar,\n";
            std::cout << "membawa kemakmuran bagi semua. Kamu adalah pahlawan!\n";
            std::cout << "--- TAMAT ---\n";
            isGameRunning = false;
            pause();
        }
    }

    return 0;
}
