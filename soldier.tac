#supply HQ
#supply Operations

# TacticLang Complete Feature Demonstration
# This file showcases all language features

# Global variable declarations
troop globalForces = 100;
ammo globalSupplies = 500.75;
codename missionName = "Operation Thunder";
status missionActive = true;

# Function: Calculate battle score
tactic calculateScore(troop kills, troop losses) {
    troop score = 0;
    
    evaluate (kills > losses) {
        score = kills - losses;
        brief "Positive score achieved!";
    }
    adjust {
        score = 0;
        brief "Mission failed - no score";
    }
    
    retreat score;
}

# Function: Deploy units in waves
tactic deployWaves(troop totalWaves, troop unitsPerWave) {
    troop totalDeployed = 0;
    
    deploy (troop wave = 0; wave < totalWaves; wave = wave + 1) {
        brief "Deploying wave " + wave;
        
        deploy (troop unit = 0; unit < unitsPerWave; unit = unit + 1) {
            totalDeployed = totalDeployed + 1;
            
            evaluate (unit % 3 == 0) {
                brief "  Unit " + unit + " - Heavy infantry";
            }
            adjust evaluate (unit % 3 == 1) {
                brief "  Unit " + unit + " - Light infantry";
            }
            adjust {
                brief "  Unit " + unit + " - Support unit";
            }
        }
    }
    
    retreat totalDeployed;
}

# Function: Patrol area
tactic patrolArea(troop maxPatrols) {
    troop patrols = 0;
    status threatDetected = false;
    
    maintain (patrols < maxPatrols && !threatDetected) {
        brief "Patrol #" + patrols + " in progress...";
        
        evaluate (patrols == 3) {
            brief "Threat detected! Aborting patrol!";
            threatDetected = true;
            abort;
        }
        
        patrols = patrols + 1;
    }
    
    retreat patrols;
}

# Function: Resource management
tactic manageResources(ammo supplies, troop troops) {
    ammo suppliesPerTroop = supplies / troops;
    
    brief "Supplies per troop: " + suppliesPerTroop;
    
    evaluate (suppliesPerTroop >= 5.0) {
        brief "Sufficient supplies!";
        retreat 1;
    }
    adjust evaluate (suppliesPerTroop >= 2.0) {
        brief "Limited supplies - ration carefully";
        retreat 0;
    }
    adjust {
        brief "Critical supply shortage!";
        retreat -1;
    }
}

# Function: Combat simulation
tactic combatSimulation(troop friendlyUnits, troop enemyUnits) {
    troop casualties = 0;
    troop enemiesDestroyed = 0;
    
    brief "Initiating combat with " + friendlyUnits + " vs " + enemyUnits;
    
    maintain (friendlyUnits > 0 && enemyUnits > 0) {
        # Simulate battle rounds
        evaluate (friendlyUnits > enemyUnits) {
            enemyUnits = enemyUnits - 1;
            enemiesDestroyed = enemiesDestroyed + 1;
            brief "Enemy unit eliminated! Remaining: " + enemyUnits;
        }
        adjust {
            friendlyUnits = friendlyUnits - 1;
            casualties = casualties + 1;
            brief "Friendly casualty! Remaining: " + friendlyUnits;
        }
        
        # Check for retreat conditions
        evaluate (casualties > 10) {
            brief "Too many casualties - tactical retreat!";
            abort;
        }
    }
    
    evaluate (enemyUnits == 0) {
        brief "Victory! All enemy forces destroyed!";
    }
    adjust {
        brief "Defeat - friendly forces depleted";
    }
    
    retreat enemiesDestroyed;
}

# Function: Test all operators
tactic testOperators() {
    troop a = 10;
    troop b = 5;
    troop result = 0;
    
    # Arithmetic operators
    result = a + b;
    brief "Addition: " + a + " + " + b + " = " + result;
    
    result = a - b;
    brief "Subtraction: " + a + " - " + b + " = " + result;
    
    result = a * b;
    brief "Multiplication: " + a + " * " + b + " = " + result;
    
    result = a / b;
    brief "Division: " + a + " / " + b + " = " + result;
    
    result = a % b;
    brief "Modulo: " + a + " % " + b + " = " + result;
    
    # Comparison operators
    status comp = false;
    
    comp = (a == b);
    brief "Equal: " + a + " == " + b + " is " + comp;
    
    comp = (a != b);
    brief "Not equal: " + a + " != " + b + " is " + comp;
    
    comp = (a < b);
    brief "Less than: " + a + " < " + b + " is " + comp;
    
    comp = (a > b);
    brief "Greater than: " + a + " > " + b + " is " + comp;
    
    comp = (a <= b);
    brief "Less or equal: " + a + " <= " + b + " is " + comp;
    
    comp = (a >= b);
    brief "Greater or equal: " + a + " >= " + b + " is " + comp;
    
    # Logical operators
    status flag1 = true;
    status flag2 = false;
    
    comp = (flag1 && flag2);
    brief "AND: true && false = " + comp;
    
    comp = (flag1 || flag2);
    brief "OR: true || false = " + comp;
    
    comp = !flag1;
    brief "NOT: !true = " + comp;
}

# Main campaign function
tactic campaign() {
    brief "====================================";
    brief "  TACTICLANG COMBAT SIMULATOR";
    brief "====================================";
    brief "";
    
    codename commanderName;
    troop initialForces;
    
    brief "Enter commander name: ";
    intel commanderName;
    
    brief "Enter initial force count: ";
    intel initialForces;
    
    brief "";
    brief "Welcome, " + commanderName + "!";
    brief "Mission: " + missionName;
    brief "Initial forces: " + initialForces;
    brief "";
    
    # Test operators
    brief "--- Testing Operators ---";
    testOperators();
    brief "";
    
    # Deploy units
    brief "--- Deploying Units ---";
    troop deployed = deployWaves(2, 3);
    brief "Total units deployed: " + deployed;
    brief "";
    
    # Patrol mission
    brief "--- Patrol Mission ---";
    troop patrolsCompleted = patrolArea(10);
    brief "Patrols completed: " + patrolsCompleted;
    brief "";
    
    # Resource management
    brief "--- Resource Management ---";
    troop resStatus = manageResources(globalSupplies, initialForces);
    brief "Resource status code: " + resStatus;
    brief "";
    
    # Combat simulation
    brief "--- Combat Simulation ---";
    troop kills = combatSimulation(initialForces, 8);
    brief "Enemies destroyed: " + kills;
    brief "";
    
    # Calculate final score
    brief "--- Mission Summary ---";
    troop finalScore = calculateScore(kills, 3);
    brief "Final mission score: " + finalScore;
    
    evaluate (finalScore > 0) {
        brief "Mission Status: SUCCESS";
        brief commanderName + " has proven worthy!";
    }
    adjust {
        brief "Mission Status: FAILED";
        brief "Better luck next time, " + commanderName;
    }
    
    brief "";
    brief "====================================";
    brief "  SIMULATION COMPLETE";
    brief "====================================";
    
    retreat 0;
}