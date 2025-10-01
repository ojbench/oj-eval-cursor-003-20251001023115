#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <sstream>

using namespace std;

struct Submission {
    string problem;
    string status;
    int time;
    
    Submission(string p, string s, int t) : problem(p), status(s), time(t) {}
};

struct ProblemStatus {
    bool solved = false;
    int wrong_attempts = 0;
    int solve_time = 0;
    int frozen_submissions = 0;
    vector<Submission> frozen_subs;
};

struct Team {
    string name;
    int solved_count = 0;
    int penalty = 0;
    vector<int> solve_times;
    map<char, ProblemStatus> problems;
    vector<Submission> all_submissions;
    int ranking = 0;
    
    Team(string n) : name(n) {}
};

class ICPCSystem {
private:
    map<string, Team*> teams;
    vector<Team*> scoreboard;
    bool started = false;
    bool frozen = false;
    int duration_time = 0;
    int problem_count = 0;
    int freeze_time = -1;
    
    bool compareTeams(Team* a, Team* b) {
        // More solved problems is better
        if (a->solved_count != b->solved_count) {
            return a->solved_count > b->solved_count;
        }
        // Less penalty is better
        if (a->penalty != b->penalty) {
            return a->penalty < b->penalty;
        }
        // Compare solve times: max first, then second max, etc.
        // solve_times are kept sorted in descending order
        int min_size = min(a->solve_times.size(), b->solve_times.size());
        for (int i = 0; i < min_size; i++) {
            if (a->solve_times[i] != b->solve_times[i]) {
                return a->solve_times[i] < b->solve_times[i];  // Smaller max time is better
            }
        }
        if (a->solve_times.size() != b->solve_times.size()) {
            return a->solve_times.size() > b->solve_times.size();
        }
        // Team name lexicographically
        return a->name < b->name;
    }
    
    void updateRankings() {
        sort(scoreboard.begin(), scoreboard.end(), [this](Team* a, Team* b) {
            return compareTeams(a, b);
        });
        for (int i = 0; i < scoreboard.size(); i++) {
            scoreboard[i]->ranking = i + 1;
        }
    }
    
    void printProblemStatus(Team* team, char prob) {
        auto it = team->problems.find(prob);
        if (it == team->problems.end()) {
            cout << ".";
            return;
        }
        
        ProblemStatus& ps = it->second;
        if (ps.solved && ps.frozen_submissions == 0) {
            // Solved and not frozen
            if (ps.wrong_attempts == 0) {
                cout << "+";
            } else {
                cout << "+" << ps.wrong_attempts;
            }
        } else if (ps.frozen_submissions > 0) {
            // Frozen
            if (ps.wrong_attempts == 0) {
                cout << "0/" << ps.frozen_submissions;
            } else {
                cout << "-" << ps.wrong_attempts << "/" << ps.frozen_submissions;
            }
        } else if (ps.wrong_attempts > 0) {
            // Not solved, has wrong attempts
            cout << "-" << ps.wrong_attempts;
        } else {
            cout << ".";
        }
    }
    
    void printScoreboard() {
        for (Team* team : scoreboard) {
            cout << team->name << " " << team->ranking << " " 
                 << team->solved_count << " " << team->penalty;
            for (int i = 0; i < problem_count; i++) {
                cout << " ";
                printProblemStatus(team, 'A' + i);
            }
            cout << "\n";
        }
    }

public:
    void addTeam(string name) {
        if (started) {
            cout << "[Error]Add failed: competition has started.\n";
            return;
        }
        if (teams.find(name) != teams.end()) {
            cout << "[Error]Add failed: duplicated team name.\n";
            return;
        }
        teams[name] = new Team(name);
        scoreboard.push_back(teams[name]);
        cout << "[Info]Add successfully.\n";
    }
    
    void startCompetition(int duration, int problems) {
        if (started) {
            cout << "[Error]Start failed: competition has started.\n";
            return;
        }
        started = true;
        duration_time = duration;
        problem_count = problems;
        // Initial ranking by team name
        updateRankings();
        cout << "[Info]Competition starts.\n";
    }
    
    void submit(char problem, string team_name, string status, int time) {
        Team* team = teams[team_name];
        team->all_submissions.push_back(Submission(string(1, problem), status, time));
        
        ProblemStatus& ps = team->problems[problem];
        
        if (frozen && !ps.solved) {
            // Add to frozen submissions
            ps.frozen_submissions++;
            ps.frozen_subs.push_back(Submission(string(1, problem), status, time));
        } else if (!frozen && !ps.solved) {
            // Normal submission
            if (status == "Accepted") {
                ps.solved = true;
                ps.solve_time = time;
                team->solved_count++;
                team->penalty += 20 * ps.wrong_attempts + time;
                // Insert in descending order
                auto pos = lower_bound(team->solve_times.begin(), team->solve_times.end(), time, greater<int>());
                team->solve_times.insert(pos, time);
            } else {
                ps.wrong_attempts++;
            }
        }
        // If already solved, ignore
    }
    
    void flush() {
        updateRankings();
        cout << "[Info]Flush scoreboard.\n";
    }
    
    void freeze() {
        if (frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            return;
        }
        frozen = true;
        cout << "[Info]Freeze scoreboard.\n";
    }
    
    void scroll() {
        if (!frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }
        
        cout << "[Info]Scroll scoreboard.\n";
        
        // First flush the scoreboard
        updateRankings();
        printScoreboard();
        
        // Process frozen problems
        while (true) {
            // Find the lowest-ranked team with frozen problems
            Team* target = nullptr;
            for (int i = scoreboard.size() - 1; i >= 0; i--) {
                Team* team = scoreboard[i];
                bool has_frozen = false;
                for (auto& p : team->problems) {
                    if (p.second.frozen_submissions > 0) {
                        has_frozen = true;
                        break;
                    }
                }
                if (has_frozen) {
                    target = team;
                    break;
                }
            }
            
            if (target == nullptr) break;
            
            // Find the smallest problem number with frozen status
            char min_problem = 'Z' + 1;
            for (auto& p : target->problems) {
                if (p.second.frozen_submissions > 0 && p.first < min_problem) {
                    min_problem = p.first;
                }
            }
            
            int old_ranking = target->ranking;
            
            // Process frozen submissions for this problem
            ProblemStatus& ps = target->problems[min_problem];
            for (auto& sub : ps.frozen_subs) {
                if (!ps.solved) {
                    if (sub.status == "Accepted") {
                        ps.solved = true;
                        ps.solve_time = sub.time;
                        target->solved_count++;
                        target->penalty += 20 * ps.wrong_attempts + sub.time;
                        // Insert in descending order
                        auto pos = lower_bound(target->solve_times.begin(), target->solve_times.end(), sub.time, greater<int>());
                        target->solve_times.insert(pos, sub.time);
                    } else {
                        ps.wrong_attempts++;
                    }
                }
            }
            ps.frozen_submissions = 0;
            ps.frozen_subs.clear();
            
            // Find new position before sorting
            int new_rank = 1;
            for (Team* t : scoreboard) {
                if (t != target && compareTeams(t, target)) {
                    new_rank++;
                }
            }
            
            string replaced_name = "";
            if (new_rank < old_ranking) {
                replaced_name = scoreboard[new_rank - 1]->name;  // 0-indexed
            }
            
            // Update rankings
            updateRankings();
            
            // Check if ranking changed and output
            if (!replaced_name.empty()) {
                cout << target->name << " " << replaced_name << " " 
                     << target->solved_count << " " << target->penalty << "\n";
            }
        }
        
        printScoreboard();
        frozen = false;
    }
    
    void queryRanking(string name) {
        if (teams.find(name) == teams.end()) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
            return;
        }
        cout << "[Info]Complete query ranking.\n";
        if (frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }
        cout << name << " NOW AT RANKING " << teams[name]->ranking << "\n";
    }
    
    void querySubmission(string team_name, string problem, string status) {
        if (teams.find(team_name) == teams.end()) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }
        
        cout << "[Info]Complete query submission.\n";
        Team* team = teams[team_name];
        
        Submission* result = nullptr;
        for (auto it = team->all_submissions.rbegin(); it != team->all_submissions.rend(); ++it) {
            bool match = true;
            if (problem != "ALL" && it->problem != problem) match = false;
            if (status != "ALL" && it->status != status) match = false;
            if (match) {
                result = &(*it);
                break;
            }
        }
        
        if (result == nullptr) {
            cout << "Cannot find any submission.\n";
        } else {
            cout << team_name << " " << result->problem << " " 
                 << result->status << " " << result->time << "\n";
        }
    }
    
    void end() {
        cout << "[Info]Competition ends.\n";
    }
};

int main() {
    ICPCSystem system;
    string line;
    
    while (getline(cin, line)) {
        istringstream iss(line);
        string cmd;
        iss >> cmd;
        
        if (cmd == "ADDTEAM") {
            string name;
            iss >> name;
            system.addTeam(name);
        } else if (cmd == "START") {
            string duration_word, problem_word;
            int duration, problems;
            iss >> duration_word >> duration >> problem_word >> problems;
            system.startCompetition(duration, problems);
        } else if (cmd == "SUBMIT") {
            char problem;
            string by, team_name, with, status, at;
            int time;
            iss >> problem >> by >> team_name >> with >> status >> at >> time;
            system.submit(problem, team_name, status, time);
        } else if (cmd == "FLUSH") {
            system.flush();
        } else if (cmd == "FREEZE") {
            system.freeze();
        } else if (cmd == "SCROLL") {
            system.scroll();
        } else if (cmd == "QUERY_RANKING") {
            string name;
            iss >> name;
            system.queryRanking(name);
        } else if (cmd == "QUERY_SUBMISSION") {
            string team_name, where, problem_part, and_part, status_part;
            iss >> team_name >> where >> problem_part >> and_part >> status_part;
            
            string problem = problem_part.substr(8);  // Remove "PROBLEM="
            string status = status_part.substr(7);     // Remove "STATUS="
            
            system.querySubmission(team_name, problem, status);
        } else if (cmd == "END") {
            system.end();
            break;
        }
    }
    
    return 0;
}
