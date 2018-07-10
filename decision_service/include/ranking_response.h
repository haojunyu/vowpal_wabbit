#pragma once
#include <cstddef>

namespace reinforcement_learning {
  class api_status;
  class ranking_response_impl;

  struct action_prob {
    int action_id;
    float probability;
  };

  //Ranking response returned by the decision service
  class ranking_response {
  public:
    ranking_response();
    ranking_response(char const* uuid);
    ~ranking_response();

    const char* get_uuid() const; // unique id
    int get_choosen_action_id(size_t& action_id, api_status* status = nullptr) const; // id of the top action chosen by the ds
    int set_choosen_action_id(size_t id, api_status* status = nullptr); // id of the top action chosen by the ds
    void set_uuid(const char* uuid);
    void push_back(const int action_id, const float prob);
    size_t size() const;
    void set_model_id(const char* model_id);
    const char * get_model_id() const;
    void clear();

    ranking_response(ranking_response&&) noexcept;
    ranking_response& operator=(ranking_response&&) noexcept;
    ranking_response(const ranking_response&) = delete;
    ranking_response& operator=(const ranking_response&) = delete;
  private:
    ranking_response_impl* _pimpl;

  public:
    class ranking_iterator {
    public:
      ranking_iterator(ranking_response_impl*);
      ranking_iterator(ranking_response_impl*, size_t);
      ranking_iterator& operator++();
      bool operator!=(const ranking_iterator& other) const;
      action_prob operator*() const;
    private:
      ranking_response_impl* _p_resp_impl;
      size_t _idx;
    };

    ranking_iterator begin() const;
    ranking_iterator end() const;
  };
}
