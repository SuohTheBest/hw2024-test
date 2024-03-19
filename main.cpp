#include <bits/stdc++.h>

#define MOVE(id, dir) (printf("move %d %d\n",id,dir))
#define GET(id) (printf("get %d\n",id))
#define PULL(id) (printf("pull %d\n",id))
#define SHIP(s_id, b_id)(printf("ship %d %d\n",s_id,b_id))
#define GO(id) (printf("go %d\n",id))


using namespace std;

const int n = 200;
const int robot_num = 10;
const int berth_num = 10;
const int N = 210;
const int boat_num = 5;

struct Robot {
	int x, y, goods;
	int status;
	int assigned_berth;

	Robot() { assigned_berth = -1; }

	Robot(int startX, int startY) {
		x = startX;
		y = startY;
		assigned_berth = -1;
	}
} robot[robot_num + 10];

struct Berth // 泊位,地图从1开始，所以注意x和y要+1
{
	int id;
	int x;
	int y;
	int transport_time;
	int loading_speed;
	bool is_occupied;

	Berth() {
		x = -1;
		is_occupied = false;
	}

	Berth(int x, int y, int transport_time, int loading_speed) {
		this->x = x;
		this->y = y;
		this->transport_time = transport_time;
		this->loading_speed = loading_speed;
		this->is_occupied = false;
	}
} berth[berth_num + 10];

struct Boat {
	int num, id, pos, status, assigned_berth;

	Boat() :
			num(0), id(0), pos(0), status(0), assigned_berth(-1) {};
} boat[boat_num];

struct Good {
	int x, y;
	int val;
	double weight;
	int time_start;
	int assigned_berth;//货物被分配到哪个港口

	Good() = default;

	explicit Good(int time) :
			time_start(time), val(0), x(0), y(0), weight(0) {};

};

int money, boat_capacity, id;
int direction[4][2] = {{0,  1},
					   {0,  -1},
					   {-1, 0},
					   {1,  0}};
char ch[N][N];

int curr_zhen;

class Util {
public:
	static int manhattanDistance(int x1, int y1, int x2, int y2) {
		return abs(x1 - x2) + abs(y1 - y2);
	}
};

class RandomManager {
public:
	RandomManager() :
			rng(std::random_device{}()), dist(0.0, 1.0) {}

	/**0到1之间的浮点数*/
	double get_random() {
		return dist(rng);
	}

private:
	mt19937 rng;
	uniform_real_distribution<double> dist;
};

RandomManager *randomManager;

class MapManager {
	struct DistanceMap {
		uint16_t data[n + 2][n + 2];

		DistanceMap() {
			memset(data, UINT16_MAX, sizeof(data));
		}

	};

	struct EnergyMap {
		double data[n + 2][n + 2];

		EnergyMap() {
			memset(data, 0, sizeof(data));
		}
	};

public:
	MapManager() {
		vector<std::pair<uint32_t, int>> berth_distance_total;

		for (auto &t: berth) {
			if (t.x == -1)continue;
			DistanceMap *temp = new DistanceMap();
			for (int i = 0; i < 4; ++i) {
				for (int j = 0; j < 4; ++j) {
					int curr_x = 1 + t.x + i;
					int curr_y = 1 + t.y + j;
					temp->data[curr_x][curr_y] = 0;
					bfs_minimal_distance(curr_x, curr_y, temp);
				}
			}
			//write_to_disk(t.id, temp);
			distance_data[t.id] = temp;

			uint32_t total = 0;
			for (int i = 1; i <= n; ++i) {
				for (int j = 1; j <= n; ++j) {
					if (ch[i][j] != '*' && ch[i][j] != '#' && temp->data[i][j] != UINT16_MAX)
						total += temp->data[i][j];
				}
			}
			berth_distance_total.emplace_back(total, t.id);
		}
		std::sort(berth_distance_total.begin(), berth_distance_total.end());

		// 其实是找距离最远的港口，我现在觉得应该平均分配港口，避免港口堆在一起，但是代码写的一坨
		available_berth[0] = &berth[berth_distance_total[0].second];
		vector<int> selected;
		selected.push_back(berth_distance_total[0].second);
		for (int i = 1; i < 5; ++i) {
			uint32_t max = 0;
			int next_berth = -1;
			for (auto j: berth_distance_total) {
				if (find(selected.begin(), selected.end(), j.second) != selected.end())continue;
				uint32_t curr_val = 0;
				for (auto k: selected) {
					curr_val += distance_data[j.second]->data[berth[k].x + 2][berth[k].y + 2];
				}
				if (curr_val > max) {
					max = curr_val;
					next_berth = j.second;
				}
			}
			available_berth[i] = &berth[next_berth];
			selected.push_back(next_berth);
		}

		energy_map = new EnergyMap();
		for (int i = 1; i <= n; ++i) {
			for (int j = 1; j <= n; ++j) {
				if (ch[i][j] == '*' || ch[i][j] == '#')
					energy_map->data[i][j] = 5000.0;
			}
		}
	}

	bool is_in_available_berth(int boat_pos) {
		for (auto &i: available_berth) {
			if (i->id == boat_pos) {
				i->is_occupied = true;
				return true;
			}
		}
		return false;
	}

	int find_available_berth() {
		for (auto &i: available_berth) {
			if (!i->is_occupied) {
				i->is_occupied = true;
				return i->id;
			}
		}
		return -2;
	}

	Berth *available_berth[boat_num];
	DistanceMap *distance_data[berth_num + 10];
	EnergyMap *energy_map;

private:

	static void bfs_minimal_distance(uint8_t start_x, uint8_t start_y, DistanceMap *distanceMap) {
		bool visited[n + 2][n + 2];
		memset(visited, false, sizeof(visited));
		std::queue<std::pair<uint8_t, uint8_t>> q;
		q.emplace(start_x, start_y);
		visited[start_x][start_y] = true;
		distanceMap->data[start_x][start_y] = 0;

		while (!q.empty()) {
			auto curr = q.front();
			q.pop();
			uint8_t curr_x = curr.first;
			uint8_t curr_y = curr.second;
			for (int i = 0; i < 4; ++i) {
				uint8_t next_x = curr_x + direction[i][0];
				uint8_t next_y = curr_y + direction[i][1];
				if (next_x > 0 && next_x <= n && next_y > 0 && next_y <= n &&
					ch[next_x][next_y] != '*' && ch[next_x][next_y] != '#' && !visited[next_x][next_y] &&
					distanceMap->data[next_x][next_y] > distanceMap->data[curr_x][curr_y] + 1) {
					visited[next_x][next_y] = true;
					distanceMap->data[next_x][next_y] = distanceMap->data[curr_x][curr_y] + 1;
					q.emplace(next_x, next_y);
				}
			}
		}
	}

	static void write_to_disk(int curr_id, DistanceMap *map) {
		ofstream os("./" + to_string(curr_id));
		for (int i = 1; i <= n; ++i) {
			for (int j = 1; j <= n; ++j) {
				os << (int) (map->data[i][j]) << " ";
			}
			os << "\n";
		}
		os.close();
	}


};

MapManager *mapManager;

class BoatManager {
public:

	static void init_boat() {
		for (auto &i: boat) {
			if (mapManager->is_in_available_berth(i.pos))i.assigned_berth = i.pos;
		}
		for (auto &i: boat) {
			if (i.assigned_berth == -1)i.assigned_berth = mapManager->find_available_berth();
			assert(i.assigned_berth != -2);
			SHIP(i.id, i.assigned_berth);
		}
	}

	static void handle_boat_event() {
		for (auto &i: boat) {
			if (i.status == 0)continue;
			if (berth[i.assigned_berth].transport_time + curr_zhen > 14999) {
				GO(i.id);
				continue;
			}
			if (i.status == 1 && i.num == boat_capacity) {
				GO(i.id);
			}
			if (i.pos == -1) {
				SHIP(i.id, i.assigned_berth);
			}
		}
	}
};

BoatManager *boatManager;

class GoodManager {
public:

	void add_new_good(Good &good) {
		compute_good_weight(good);
		if (!good_list.empty() && good_list.end()->weight >= good.weight)return;
		else {
			good_list.push_back(good);
		}
	}

	//存储所有的物品，删除过期的物品
	void add_new_good_2(Good &good) {
		compute_good_weight(good);
		good_list.push_back(good);
	}

	//删除队列开头的过期物品
	void delete_expire_good() {
		if (!good_list.empty()) {
			while (curr_zhen - good_list.begin()->time_start > 999)good_list.pop_front();
		}
	}

//	void compute_priority() {
//		//std::sort(good_list.begin(), good_list.end(), cmp);
//		int size = good_list.size();
//		while (size > 20) {
//			good_list.pop_back();
//			size--;
//		}
//	};

	double compute_energy(Good& good,int dx, int dy) const {
		int distance = abs(dx - good.x) + abs(dy - good.y);
		if (distance > 100)return 0;
		else return 1.0 * good.val / distance;
	}

	list<Good> good_list;
private:
	static bool cmp(Good &a, Good &b) {
		double wa = (curr_zhen - a.time_start > 999) ? -1 : a.weight;
		double wb = (curr_zhen - b.time_start > 999) ? -1 : b.weight;
		return wa > wb;
	}

	static void compute_good_weight(Good &good) {//计算权重
		double t;
		uint16_t min = UINT16_MAX;
		int berth_id = -1;
		for (auto &i: mapManager->available_berth) {
			uint16_t val = mapManager->distance_data[i->id]->data[good.x][good.y];
			if (val < min) {
				min = val;
				berth_id = i->id;
			}
		}
		t = good.val / (1.0 + min);
		good.weight = t;
		good.assigned_berth = berth_id;
	}
};

GoodManager *goodManager;

class RobotManager {
	struct Node {
		int x, y; // 节点坐标
		int g, h; // 实际代价和启发式代价
		int next; // 方向,0→	1← 2↑ 3↓
		Node *parent;//父节点

		Node(int x, int y, int g, int h, int next, Node *parent) :
				x(x), y(y), g(g), h(h), next(next), parent(parent) {}
	};

public:

	static void handle_robot_event(int robot_id) {
		if (robot[robot_id].status == 0)return;
		if (robot[robot_id].goods == 1) {
			go_to_berth(robot_id, robot[robot_id].assigned_berth);
		} else {

			auto it = goodManager->good_list.begin();
			while (it != goodManager->good_list.end()) {
				auto &j = *it;
				if (j.x == robot[robot_id].x && j.y == robot[robot_id].y) {
					GET(robot_id);
					robot[robot_id].assigned_berth = j.assigned_berth;
					go_to_berth(robot_id, robot[robot_id].assigned_berth);
					it = goodManager->good_list.erase(it);
					return;
				}

				it++;
			}
			move_to_lowest_energy(robot_id);
		}
	}


private:
	void aStar(int robot_id, int targetX, int targetY) {
		bool visited[n + 2][n + 2];
		stack<int> path;
		memset(visited, false, sizeof(visited));
		priority_queue<Node *, vector<Node *>, decltype(&comp_node)> boundary;
		boundary.push(
				new Node(robot[robot_id].x, robot[robot_id].y, 0, Util::manhattanDistance(robot[robot_id].x, robot[robot_id].x, targetX, targetY), -1, nullptr));
		while (!boundary.empty()) {
			Node *curr_node = boundary.top();
			boundary.pop();

			if (curr_node->x == targetX && curr_node->y == targetY) {
				// 找到目标位置，回溯路径
				while (curr_node != nullptr) {
					path.push(opposite_direction(curr_node->next));
					curr_node = curr_node->parent;
				}
				break;
			}

			visited[curr_node->x][curr_node->y] = true;

			// 扩展相邻节点
			for (int i = 0; i < 4; i++) {
				int new_x = curr_node->x + direction[i][0];
				int new_y = curr_node->y + direction[i][1];

				if (!visited[new_x][new_y] && ch[new_x][new_y] != '#' && ch[new_x][new_y] && new_x > 0 &&
					new_x <= 200 &&
					new_y > 0 && new_y <= 200) {
					int new_g = curr_node->g + 1; // 每次移动代价为1
					int new_h = Util::manhattanDistance(new_x, new_y, targetX, targetY);
					Node *newNode = new Node(new_x, new_y, new_g, new_h, i, curr_node);
					boundary.push(newNode);
				}
			}
		}
		MOVE(robot_id,path.top());
	}

	static void go_to_berth(int robot_id, int berth_id) {
		int min = INT_MAX;
		int next_move = -1;
		int curr_x = robot[robot_id].x;
		int curr_y = robot[robot_id].y;
		if (mapManager->distance_data[berth_id]->data[curr_x][curr_y] == 0) {
			PULL(robot_id);
			return;
		}
		for (int i = 0; i < 4; ++i) {
			int next_x = curr_x + direction[i][0];
			int next_y = curr_y + direction[i][1];
			int val = mapManager->distance_data[berth_id]->data[next_x][next_y];
			for (int j = 0; j < robot_num; ++j) {
				if (j == robot_id)continue;
				int distance = Util::manhattanDistance(next_x, next_y, robot[j].x, robot[j].y);
				if (distance == 0) {
					val = INT_MAX;
					break;
				} else if (distance < 3) {
					val += 4;
				}
			}
			if (val < min) {
				min = val;
				next_move = i;
			} else if (val == min && randomManager->get_random() > 0.5)
				next_move = i;
		}
		MOVE(robot_id, next_move);
		robot[robot_id].x += direction[next_move][0];
		robot[robot_id].y += direction[next_move][1];
	}

	static void move_to_lowest_energy(int robot_id) {
		double min_energy = 5000;
		int next_move = -1;
		//bool is_move_and_hold= false;
		for (int i = 0; i < 4; ++i) {
			int curr_x = robot[robot_id].x;
			int curr_y = robot[robot_id].y;
			int next_x = curr_x + direction[i][0];
			int next_y = curr_y + direction[i][1];
			int flag = false;
			for (int j = 0; j < robot_id; ++j) {
				if (robot[j].x == next_x && robot[j].y == next_y) {
					flag = true;
					break;
				}
			}
			if (flag || mapManager->energy_map->data[next_x][next_y] > 4000 || next_x <= 0 || next_x > 200 ||
				next_y <= 0 || next_y > 200)
				continue;
			double curr_energy = mapManager->energy_map->data[next_x][next_y];
			for (auto &j: goodManager->good_list) {
				curr_energy -= goodManager->compute_energy(j,next_x, next_y);
			}
			for (int j = 0; j < robot_num; ++j) {
				if (j == robot_id)continue;
				else if (Util::manhattanDistance(next_x, next_y, robot[j].x, robot[j].y) < 3) {
					curr_energy += 10;
				}
			}
			if (curr_energy < min_energy || (curr_energy == min_energy && randomManager->get_random() < 0.5)) {
				min_energy = curr_energy;
				next_move = i;
			}
		}
		if (next_move != -1) {
			MOVE(robot_id, next_move);
			robot[robot_id].x += direction[next_move][0];
			robot[robot_id].y += direction[next_move][1];
		}
	}

	static bool comp_node(Node *a, Node *b) {
		int f1 = a->h + a->g;
		int f2 = b->h + b->g;
		if (f1 == f2)return randomManager->get_random() < 0.5;//引入随机性，避免每次生成相同的路线，降低撞车概率
		return f1 < f2;
	};

	static int opposite_direction(int d) {
		if (d % 2 == 0)return d + 1;
		else return d - 1;
	}


};

RobotManager *robotManager;


void Init() {
	auto start = chrono::system_clock::now();
	for (int i = 1; i <= n; i++)
		scanf("%s", ch[i] + 1);

	cerr << "berth data:\n";
	for (int i = 0; i < berth_num; i++) {
		int berth_id;
		scanf("%d", &berth_id);
		berth[berth_id].id = berth_id;
		scanf("%d%d%d%d", &berth[berth_id].x, &berth[berth_id].y, &berth[berth_id].transport_time,
			  &berth[berth_id].loading_speed);// 这里的x和y都是泊位的左上角

		cerr << berth[berth_id].id << " " << berth[berth_id].x << " " << berth[berth_id].y << " "
			 << berth[berth_id].transport_time << " "
			 << berth[berth_id].loading_speed << endl;
	}
	scanf("%d", &boat_capacity);

	cerr << "boat capacity = " << boat_capacity << endl;

	char okk[50];
	scanf("%s", okk);
	assert(okk[0] == 'O' && okk[1] == 'K');
	randomManager = new RandomManager();
	mapManager = new MapManager();
	boatManager = new BoatManager();
	robotManager = new RobotManager();
	goodManager = new GoodManager();
	for (int i = 0; i < 5; ++i) {
		boat[i].id = i;
	}
	auto end = chrono::system_clock::now();
	std::chrono::duration<double> elapsed_time = end - start;
	std::cerr << "init time = " << elapsed_time.count() << endl;
	printf("OK\n");
	fflush(stdout);
}

int Input(int zhen) {
	scanf("%d%d", &id, &money);
	int num;
	scanf("%d", &num);
	for (int i = 1; i <= num; i++) {
		int x, y, val;
		scanf("%d%d%d", &x, &y, &val);
		Good t(zhen);
		t.x = x + 1;
		t.y = y + 1;
		t.val = val;
		goodManager->add_new_good_2(t);
	}
	for (int i = 0; i < robot_num; i++) {
		scanf("%d%d%d%d", &robot[i].goods, &robot[i].x, &robot[i].y, &robot[i].status);
		robot[i].x += 1;
		robot[i].y += 1;
	}
	for (int i = 0; i < boat_num; i++) {
		scanf("%d%d\n", &boat[i].status, &boat[i].pos);
	}
	char okk[50];
	scanf("%s", okk);
	assert(okk[0] == 'O' && okk[1] == 'K');
	return id;
}

int main() {
	Init();
	for (curr_zhen = 1; curr_zhen <= 15000; curr_zhen++) {
		int id = Input(curr_zhen);
		if (curr_zhen != 1)
			boatManager->handle_boat_event();
		else {
			boatManager->init_boat();
			cerr << "robot data:" << endl;
			for (int i = 0; i < robot_num; ++i) {
				cerr << robot[i].x << "\t" << robot[i].y << "\t" << ch[robot[i].x][robot[i].y] << endl;
			}

		}

		for (int i = 0; i < robot_num; i++) {
			robotManager->handle_robot_event(i);
		}
		puts("OK");
		fflush(stdout);
	}
	return 0;
}
