#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>

// === Encryption Utilities ===
const std::string KEY = "supersecret";
std::string xorCipher(const std::string &data, const std::string &key) {
    std::string out = data;
    for (size_t i = 0; i < data.size(); ++i)
        out[i] = data[i] ^ key[i % key.size()];
    return out;
}
std::string toHex(const std::string &data) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char c : data)
        oss << std::setw(2) << (int)c;
    return oss.str();
}
std::string fromHex(const std::string &hex) {
    std::string out; out.reserve(hex.size()/2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        out.push_back((char)std::stoi(hex.substr(i,2), nullptr, 16));
    }
    return out;
}

// === File Names ===
const std::string STUDENT_FILE = "students.txt";
const std::string USER_FILE    = "login.txt";

// === Data Structures ===
struct Student {
    int id;
    std::string name;
    std::vector<double> grades;
};
struct User {
    std::string username;
    std::string password;
    std::string role;   // "PROF" or "STUD"
    int studentId;      // only used if role=="STUD"
};

// === Helpers ===
std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> parts;
    std::istringstream iss(s);
    std::string tok;
    while (std::getline(iss, tok, delim))
        parts.push_back(tok);
    return parts;
}

// === Student Serialization ===
std::string serializeStudent(const Student &s) {
    std::ostringstream oss;
    oss << s.id << '|' << s.name << '|';
    for (size_t i = 0; i < s.grades.size(); ++i) {
        oss << s.grades[i] << (i+1<s.grades.size()?",":"");
    }
    return oss.str();
}
Student deserializeStudent(const std::string &plain) {
    Student s;
    auto parts = split(plain, '|');
    s.id   = std::stoi(parts[0]);
    s.name = parts[1];
    std::istringstream gs(parts[2]);
    std::string g; 
    while (std::getline(gs, g, ',')) 
        s.grades.push_back(std::stod(g));
    return s;
}

// === User Serialization ===
// Format: username|password|role|studentId
std::string serializeUser(const User &u) {
    std::ostringstream oss;
    oss << u.username << '|' << u.password << '|' << u.role << '|' << u.studentId;
    return oss.str();
}
User deserializeUser(const std::string &plain) {
    auto parts = split(plain, '|');
    User u;
    u.username  = parts[0];
    u.password  = parts[1];
    u.role      = parts[2];
    u.studentId = (parts.size()>3 ? std::stoi(parts[3]) : 0);
    return u;
}

// === File I/O ===
std::vector<Student> loadStudents() {
    std::vector<Student> out;
    std::ifstream ifs(STUDENT_FILE);
    std::string hex;
    while (std::getline(ifs, hex)) {
        if (hex.empty()) continue;
        std::string crypt = fromHex(hex);
        std::string plain = xorCipher(crypt, KEY);
        out.push_back(deserializeStudent(plain));
    }
    return out;
}
void saveAllStudents(const std::vector<Student> &list) {
    std::ofstream ofs(STUDENT_FILE, std::ios::trunc);
    for (auto &s : list) {
        auto crypt = xorCipher(serializeStudent(s), KEY);
        ofs << toHex(crypt) << "\n";
    }
}

std::vector<User> loadUsers() {
    std::vector<User> out;
    std::ifstream ifs(USER_FILE);
    std::string hex;
    while (std::getline(ifs, hex)) {
        if (hex.empty()) continue;
        std::string crypt = fromHex(hex);
        std::string plain = xorCipher(crypt, KEY);
        out.push_back(deserializeUser(plain));
    }
    return out;
}
void saveAllUsers(const std::vector<User> &list) {
    std::ofstream ofs(USER_FILE, std::ios::trunc);
    for (auto &u : list) {
        auto crypt = xorCipher(serializeUser(u), KEY);
        ofs << toHex(crypt) << "\n";
    }
}

// === Authentication ===
bool authenticate(User &current) {
    auto users = loadUsers();
    std::string usr, pwd;
    std::cout << "Username: "; std::getline(std::cin, usr);
    std::cout << "Password: "; std::getline(std::cin, pwd);
    for (auto &u : users) {
        if (u.username==usr && u.password==pwd) {
            current = u;
            return true;
        }
    }
    return false;
}

// === Professor Functions ===
void addStudent() {
    Student s;
    std::cout<<"ID: "; std::cin>>s.id; std::cin.ignore();
    std::cout<<"Name: "; std::getline(std::cin, s.name);
    std::cout<<"Grades (space-separated): ";
    std::string line; std::getline(std::cin, line);
    std::istringstream gs(line);
    double g; while(gs>>g) s.grades.push_back(g);
    auto students = loadStudents();
    students.push_back(s);
    saveAllStudents(students);
    std::cout<<"[+] Student added.\n";
}
void removeStudent() {
    int id; std::cout<<"ID to remove: "; std::cin>>id; std::cin.ignore();
    auto students = loadStudents();
    students.erase(std::remove_if(students.begin(), students.end(),
        [&](auto &st){return st.id==id;}), students.end());
    saveAllStudents(students);
    std::cout<<"[+] Student removed (if existed).\n";
}
void updateGrades() {
    int id; std::cout<<"ID to update: "; std::cin>>id; std::cin.ignore();
    auto students = loadStudents();
    for (auto &st: students) {
        if (st.id==id) {
            std::cout<<"New grades (space-separated): ";
            std::string line; std::getline(std::cin, line);
            st.grades.clear();
            std::istringstream gs(line);
            double g; while(gs>>g) st.grades.push_back(g);
            break;
        }
    }
    saveAllStudents(students);
    std::cout<<"[+] Grades updated.\n";
}
void viewAllStudents() {
    auto students = loadStudents();
    std::cout<<"\n-- All Students --\n";
    for (auto &st: students) {
        std::cout<<st.id<<" | "<<st.name<<" | ";
        for (size_t i=0;i<st.grades.size();++i)
            std::cout<<st.grades[i]<<(i+1<st.grades.size()?", ":"");
        std::cout<<"\n";
    }
}
void searchStudent() {
    std::cout<<"Search by (1) ID or (2) Name? "; int c; std::cin>>c; std::cin.ignore();
    auto students = loadStudents();
    if (c==1) {
        int id; std::cout<<"ID: "; std::cin>>id; std::cin.ignore();
        for (auto &st: students)
            if (st.id==id) { std::cout<<st.id<<" | "<<st.name<<"\n"; break;}
    } else {
        std::string nm; std::cout<<"Name substring: "; std::getline(std::cin, nm);
        for (auto &st: students)
            if (st.name.find(nm)!=std::string::npos)
                std::cout<<st.id<<" | "<<st.name<<"\n";
    }
}
void generateReport() {
    auto students = loadStudents();
    std::ofstream ofs("report.csv");
    ofs<<"ID,Name,Average\n";
    for (auto &st: students) {
        double sum=0;
        for (auto &g: st.grades) sum+=g;
        double avg = st.grades.empty()?0:sum/st.grades.size();
        ofs<<st.id<<","<<st.name<<","<<avg<<"\n";
    }
    std::cout<<"[+] Report generated: report.csv\n";
}
void resetStudentPassword() {
    std::string un; std::cout<<"Student username: "; std::getline(std::cin, un);
    std::string np; std::cout<<"New password: ";  std::getline(std::cin, np);
    auto users = loadUsers();
    for (auto &u: users) {
        if (u.username==un && u.role=="STUD") {
            u.password = np;
            saveAllUsers(users);
            std::cout<<"[+] Password reset.\n"; 
            return;
        }
    }
    std::cout<<"[-] Student not found.\n";
}

// === Student Functions ===
void viewProfile(const User &me) {
    auto students = loadStudents();
    for (auto &st: students) {
        if (st.id==me.studentId) {
            std::cout<<"ID: "<<st.id<<"\nName: "<<st.name<<"\n";
            return;
        }
    }
    std::cout<<"[-] Profile not found.\n";
}
void viewGrades(const User &me) {
    auto students = loadStudents();
    for (auto &st: students) {
        if (st.id==me.studentId) {
            std::cout<<"Grades: ";
            for (size_t i=0;i<st.grades.size();++i)
                std::cout<<st.grades[i]<<(i+1<st.grades.size()?", ":"");
            std::cout<<"\n"; 
            return;
        }
    }
    std::cout<<"[-] Grades not found.\n";
}
void changeOwnPassword(User &me) {
    std::string np; std::cout<<"New password: "; std::getline(std::cin, np);
    auto users = loadUsers();
    for (auto &u: users) {
        if (u.username==me.username) {
            u.password = np;
            saveAllUsers(users);
            me.password = np;
            std::cout<<"[+] Your password has been changed.\n";
            return;
        }
    }
}
void exportTranscript(const User &me) {
    auto students = loadStudents();
    std::ofstream ofs("my_transcript.txt");
    for (auto &st: students) {
        if (st.id==me.studentId) {
            ofs<<"ID: "<<st.id<<"\nName: "<<st.name<<"\nGrades: ";
            for (size_t i=0;i<st.grades.size();++i)
                std::cout<<st.grades[i]<<(i+1<st.grades.size()?", ":"");
            ofs<<"\n";
            std::cout<<"[+] Transcript exported to my_transcript.txt\n";
            return;
        }
    }
    std::cout<<"[-] Could not export transcript.\n";
}

// === Menus ===
void professorMenu() {
    while (true) {
        std::cout<<"\n[Professor Menu]\n"
                 "1) Add Student\n2) Remove Student\n"
                 "3) Update Grades\n4) View All Students\n"
                 "5) Search Student\n6) Generate Report\n"
                 "7) Reset Student Password\n8) Exit\n"
                 "Choice: ";
        int c; if (!(std::cin>>c)) return;
        std::cin.ignore();
        switch(c){
            case 1: addStudent(); break;
            case 2: removeStudent(); break;
            case 3: updateGrades(); break;
            case 4: viewAllStudents(); break;
            case 5: searchStudent(); break;
            case 6: generateReport(); break;
            case 7: resetStudentPassword(); break;
            default: return;
        }
    }
}
void studentMenu(User &me) {
    while (true) {
        std::cout<<"\n[Student Menu]\n"
                 "1) View Profile\n2) View Grades\n"
                 "3) Change Password\n4) Export Transcript\n5) Exit\n"
                 "Choice: ";
        int c; if (!(std::cin>>c)) return;
        std::cin.ignore();
        switch(c){
            case 1: viewProfile(me); break;
            case 2: viewGrades(me); break;
            case 3: changeOwnPassword(me); break;
            case 4: exportTranscript(me); break;
            default: return;
        }
    }
}
void signup() {
    User u;
    std::cout << "[ Signup ]\n";
    std::cout << "Username: ";    std::getline(std::cin, u.username);
    std::cout << "Password: ";    std::getline(std::cin, u.password);
    std::cout << "Role (PROF/STUD): "; std::getline(std::cin, u.role);
    if (u.role == "STUD") {
        std::cout << "Student ID: ";
        std::cin >> u.studentId;
        std::cin.ignore();
    } else {
        u.studentId = 0;
    }
    auto users = loadUsers();
    users.push_back(u);
    saveAllUsers(users);
    std::cout << "[+] Account created – please login.\n\n";
}


// === Main Program ===
int main(){
    std::cout << "=== College Management System ===\n";
    std::cout << "1) Login\n2) Signup\nChoose: ";
    int choice; std::cin >> choice; std::cin.ignore();
    if (choice == 2) signup();
    std::cout << "Login Required\n";
    User current;
    if (!authenticate(current)) {
        std::cout<<"[-] Invalid credentials. Exiting.\n";
        return 1;
    }
    std::cout<<"[+] Welcome, "<<current.username
             <<" ("<<current.role<<")\n";

    if (current.role == "PROF") {
        professorMenu();
    } else {
        studentMenu(current);
    }
    std::cout<<"Goodbye!\n";
    return 0;
}
