#include "Arduino.h"

enum Hx711Gain {
  HX711_GAIN_128 = 1,
  HX711_GAIN_32 = 2,
  HX711_GAIN_64 = 3,
  HX711_NUM_GAIN
};

struct Hx711Node {
  int id;
  int pin_dout;
  int pin_sck;
  enum Hx711Gain gain;
  int32_t raw_data;
  int32_t offset;
  struct Hx711Node* next_node;
  struct Hx711Node* prev_node;
};

class Hx711 {
  public:
    Hx711();
    ~Hx711();
    void PrintNodeInfo(void);
    int InsertNode(int pin_dout, int pin_sck);
    int DeleteNode(int id);
    void UpdateNode(int id);
    void UpdateAllNode(enum Hx711Gain gain);
    void UpdateOffset(enum Hx711Gain gain, int num_iter);
    int32_t GetNodeRawData(int id);
    int32_t GetNodeData(int id);
    
  private:
    struct Hx711Node* FindNode(int id);
    
    const int kTimeDoutFallingEdgeToSckRisingEdge = 1; // us
    const int kTimeSckRisingEdgeToDoutReady = 1; // us
    const int kTimeSckHigh = 1; // us
    const int kTimeSckLow = 1; // us
    const int kTimeSckHighReset = 100; //us

    struct Hx711Node* node_list;
    int max_id;
};

Hx711::Hx711() {
  node_list = NULL;
}

Hx711::~Hx711() {
  struct Hx711Node* node = node_list;
  while(node != NULL) {
    node_list = node_list->next_node;
    free(node);
    node = node_list;
  }
}

void Hx711::PrintNodeInfo(void) {
  struct Hx711Node* node = node_list;
  while(node != NULL) {
    Serial.print("Node(");
    Serial.print(node->id);
    Serial.print(", ");
    Serial.print(node->pin_dout);
    Serial.print(", ");
    Serial.print(node->pin_sck);
    Serial.print(", ");
    Serial.print(node->gain);
    Serial.print(")");
    Serial.println();
    node = node->next_node;
  }
}

int Hx711::InsertNode(int pin_dout, int pin_sck) {
  // New node
  struct Hx711Node* node = malloc(sizeof(struct Hx711Node));
  node->id = ++max_id;
  node->pin_dout = pin_dout;
  node->pin_sck = pin_sck;
  node->gain = HX711_GAIN_128;
  node->raw_data = 0;
  node->offset = 0;
  // Setup digital pin
  pinMode(node->pin_dout, INPUT);
  pinMode(node->pin_sck, OUTPUT);
  // Reset
  digitalWrite(node->pin_sck, HIGH);
  delayMicroseconds(kTimeSckHighReset);
  digitalWrite(node->pin_sck, LOW);
  // Insert new node to node_list
  node->next_node = node_list;
  node_list->prev_node = node;
  node_list = node;
  // Return new node ID
  return node->id;
}

int Hx711::DeleteNode(int id) {
  struct Hx711Node* node = FindNode(id);
  if (node != NULL) {
    node->prev_node->next_node = node->next_node;
    node->next_node->prev_node = node->prev_node;
    free(node);
    return id;
  } else {
    return -1;
  }
}

void Hx711::UpdateNode(int id) {
  struct Hx711Node* node = FindNode(id);
  if (node == NULL) return;
  node->raw_data = 0;
  // Data ready;
  while (digitalRead(node->pin_dout) == HIGH) {
    delayMicroseconds(1);
  }
  // Delay DOUT falling edge to PD_SCK rising edge
  delayMicroseconds(kTimeDoutFallingEdgeToSckRisingEdge);
  // Read 24bit Data
  for (int i = 0; i < 24; i++) {
    digitalWrite(node->pin_sck, HIGH);
    delayMicroseconds(kTimeSckHigh);
    digitalWrite(node->pin_sck, LOW);
    node->raw_data <<= 1;
    node->raw_data |= digitalRead(node->pin_dout) == HIGH ? 1 : 0;
    delayMicroseconds(kTimeSckLow);
  }
  for (int i = 0; i < node->gain; i++) {
    digitalWrite(node->pin_sck, HIGH);
    delayMicroseconds(kTimeSckHigh);
    digitalWrite(node->pin_sck, LOW);
    delayMicroseconds(kTimeSckLow);
  }
  // MSB
  if (node->raw_data & ((int32_t)0x01 << 23)) {
    node->raw_data |= 0xFF000000;
  }
}

void Hx711::UpdateAllNode(enum Hx711Gain gain) {
  struct Hx711Node* node;
  // Clear raw data
  node = node_list;
  while (node != NULL) {
    node->raw_data = 0;
    node = node->next_node;
  }
  // Data ready;
  node = node_list;
  while (node != NULL) {
    if (digitalRead(node->pin_dout) == HIGH) {
      delayMicroseconds(1);
      continue;
    }
    node = node->next_node;
  }
  // Delay DOUT falling edge to PD_SCK rising edge
  delayMicroseconds(kTimeDoutFallingEdgeToSckRisingEdge);
  // Read 24bit Data
  for (int i = 0; i < 24; i++) {
    node = node_list;
    while (node != NULL) {
      digitalWrite(node->pin_sck, HIGH);
      node = node->next_node;
    }
    delayMicroseconds(kTimeSckHigh);
    node = node_list;
    while (node != NULL) {
      digitalWrite(node->pin_sck, LOW);
      node = node->next_node;
    }
    node = node_list;
    while (node != NULL) {
      node->raw_data <<= 1;
      node->raw_data |= digitalRead(node->pin_dout) == HIGH ? 1 : 0;
      node = node->next_node;
    }
    delayMicroseconds(kTimeSckLow);
  }
  for (int i = 0; i < gain; i++) {
    node = node_list;
    while (node != NULL) {
      digitalWrite(node->pin_sck, HIGH);
      node = node->next_node;
    }
    delayMicroseconds(kTimeSckHigh);
    node = node_list;
    while (node != NULL) {
      digitalWrite(node->pin_sck, LOW);
      node = node->next_node;
    }
    delayMicroseconds(kTimeSckLow);
  }
  // MSB
  node = node_list;
  while (node != NULL) {
    if (node->raw_data & ((int32_t)0x01 << 23)) {
      node->raw_data |= 0xFF000000;
    }
    node = node->next_node;
  }
}

void Hx711::UpdateOffset(enum Hx711Gain gain, int num_iter) {
  struct Hx711Node* node;
  int num_samples = num_iter;
  node = node_list;
  while (node != NULL) {
    node->offset = 0;
    node = node->next_node;
  }
  for (int i = 0; i < num_samples; i++) {
    UpdateAllNode(gain);
    node = node_list;
    while (node != NULL) {
      node->offset += node->raw_data;
      node = node->next_node;
    }
  }
  node = node_list;
  while (node != NULL) {
    node->offset /= num_samples;
    node = node->next_node;
  }
}

int32_t Hx711::GetNodeRawData(int id) {
  struct Hx711Node* node = FindNode(id);
  return node->raw_data;
}

int32_t Hx711::GetNodeData(int id) {
  struct Hx711Node* node = FindNode(id);
  return node->raw_data - node->offset;
}

struct Hx711Node* Hx711::FindNode(int id) {
  struct Hx711Node* node = node_list;
  while(node != NULL) {
    if (node->id == id) {
      return node;
    }
    node = node->next_node;
  }
  return NULL;
}
