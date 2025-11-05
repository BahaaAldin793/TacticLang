/* * Simplified TacticLang for current parser version * */

troop soldiers = 50;
ammo supplies = 300;

tactic showStatus() {
    brief "HQ: Troops ready and supplies confirmed.";
}

tactic simpleDeploy() {
    status deployed = 0;
    deploy(supplies > 0) {
        brief "Deployment initiated!";
    }
}

tactic checkFlags() {
    status ready = true;
    evaluate(ready) {
        brief "All flags checked successfully!";
    }
}

tactic mainCampaign() {
    showStatus();
    simpleDeploy();
    checkFlags();
}
