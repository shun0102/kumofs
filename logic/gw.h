#ifndef LOGIC_GW_H__
#define LOGIC_GW_H__

#include "logic/rpc.h"
#include "rpc/sbuffer.h"

namespace kumo {


class GatewayInterface;

class Gateway : public RPCBase<Gateway>, public rpc::client<> {
public:
	template <typename Config>
	Gateway(Config& cfg);

	~Gateway();

	void dispatch(
			shared_session& from, rpc::weak_responder response,
			method_id method, msgobj param, shared_zone& life);

	void session_lost(const address& addr, shared_session& s);

	void step_timeout();

public:
	void add_gateway(GatewayInterface* gw);

public:
	RPC_DECL(HashSpacePush);

public:
	struct basic_response {
		shared_zone life;
		int error;
	};

	struct basic_request {
		basic_request() : life(new mp::zone()) { }
		shared_zone life;
	};


	struct get_response : basic_response {
		const char* key;
		uint32_t keylen;
		char* val;
		uint32_t vallen;
		uint64_t clocktime;
	};

	struct get_request : basic_request {
		void (*callback)(void* user, get_response& res);
		void* user;
		const char* key;
		uint32_t keylen;
	};


	struct set_response : basic_response {
		const char* key;
		uint32_t keylen;
		const char* val;
		uint32_t vallen;
		uint64_t clocktime;
	};

	struct set_request : basic_request {
		void (*callback)(void* user, set_response& res);
		void* user;
		const char* key;
		uint32_t keylen;
		const char* val;
		uint32_t vallen;
	};


	struct delete_response : basic_response {
		const char* key;
		uint32_t keylen;
		bool deleted;
	};

	struct delete_request : basic_request {
		void (*callback)(void* user, delete_response& res);
		void* user;
		const char* key;
		uint32_t keylen;
	};

	void submit(get_request& req);

	void submit(set_request& req);

	void submit(delete_request& req);

private:
	void Get(void (*callback)(void*, get_response&), void* user,
			shared_zone life,
			const char* key, uint32_t keylen);

	void Set(void (*callback)(void*, set_response&), void* user,
			shared_zone life,
			const char* key, uint32_t keylen,
			const char* val, uint32_t vallen);

	void Delete(void (*callback)(void*, delete_response&), void* user,
			shared_zone life,
			const char* key, uint32_t keylen);

private:
	typedef RPC_RETRY(Get) RetryGet;
	RPC_REPLY_DECL(ResGet, from, res, err, life,
			RetryGet* retry,
			void (*callback)(void*, get_response&), void* user);

	typedef RPC_RETRY(Set) RetrySet;
	RPC_REPLY_DECL(ResSet, from, res, err, life,
			RetrySet* retry,
			void (*callback)(void*, set_response&), void* user);

	typedef RPC_RETRY(Delete) RetryDelete;
	RPC_REPLY_DECL(ResDelete, from, res, err, life,
			RetryDelete* retry,
			void (*callback)(void*, delete_response&), void* user);

	void renew_hash_space();
	void renew_hash_space_for(const address& addr);
	RPC_REPLY_DECL(ResHashSpaceRequest, from, res, err, life);

	shared_session server_for(const char* key, uint32_t keylen);
	shared_session server_for(const char* key, uint32_t keylen, unsigned int offset);

private:
	Clock m_clock;
	HashSpace m_hs;
	address m_manager1;
	address m_manager2;

	const unsigned short m_cfg_get_retry_num;
	const unsigned short m_cfg_set_retry_num;
	const unsigned short m_cfg_delete_retry_num;

private:
	shared_session get_server(const address& addr)
	{
		return get_session(addr);
	}

private:
	Gateway();
	Gateway(const Gateway&);
};


template <typename Config>
Gateway::Gateway(Config& cfg) :
	RPCBase<Gateway>(cfg),
	rpc::client<>(
			cfg.connect_timeout_steps,
			cfg.reconnect_timeout_msec),
	m_manager1(cfg.manager1),
	m_manager2(cfg.manager2),
	m_cfg_get_retry_num(cfg.get_retry_num),
	m_cfg_set_retry_num(cfg.set_retry_num),
	m_cfg_delete_retry_num(cfg.delete_retry_num)
{
	start_timeout_step<Gateway>(cfg.clock_interval_usec, this);
	renew_hash_space();
}


}  // namespace kumo

#endif /* logic/gw.h */
