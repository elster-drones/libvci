// Copyright (c) 2018-2021, AT&T Intellectual Property.
// All rights reserved.
//
// SPDX-License-Identifier: LGPL-2.1-only

#ifndef __VCI_HPP__
#define __VCI_HPP__
#include <cstdint>
#include <string>
#include <map>
#include <functional>
#include <memory>

namespace _vci {
	struct _CompImpl;
	struct _ClientImpl;
	struct _RPCCallImpl;
	struct _SubscriptionImpl;
}
namespace vci {
	typedef std::string EncodedInput;
	typedef std::string EncodedOutput;

	typedef std::function<EncodedOutput(const EncodedInput&)> MethodFn;
	typedef std::function<EncodedOutput(const EncodedInput&, const EncodedInput&)> MethodMetaFn;
	typedef std::function<void(const EncodedInput&)> SubscriberFn;

	class Component;

	class Exception {
	public:
		Exception(const std::string& app_tag,
				  const std::string& info,
				  const std::string& path);
		~Exception() {};
		std::string app_tag() const;
		std::string info() const;
		std::string path() const;
		const std::string& what() const {
			return _info;
		};
	private:
		std::string _app_tag;
		std::string _info;
		std::string _path;
	};

	class Config {
	public:
		virtual void set(
			const EncodedInput& input)  = 0;
		virtual void check(
			const EncodedInput& input)  = 0;
		virtual EncodedOutput get() { return "{}"; }
		virtual ~Config() {};
	};

	class State {
	public:
		virtual EncodedOutput get() = 0;
		virtual ~State() {};
	};

	class Method {
	public:
		virtual EncodedOutput operator()(const EncodedInput& input) = 0;
		virtual ~Method() {};
	};

	class MethodMeta {
	public:
		virtual EncodedOutput operator()(const EncodedInput& meta, const EncodedInput& input) = 0;
		virtual ~MethodMeta() {};
	};

	class Subscriber {
	public:
		virtual void operator()(const EncodedInput& input) = 0;
		virtual ~Subscriber() {};
	};

	class Model {
	public:
		Model(std::string name);
		~Model(){};
		Model& config(Config* config);
		Model& state(State* state);
		Model& rpc(const std::string& module,
				   const std::string& name,
				   Method* rpc);
		Model& rpc(const std::string& module,
				   const std::string& name, MethodFn rpc);
		Model& rpc(const std::string& module,
				   const std::string& name,
				   MethodMeta* rpc);
		Model& rpc(const std::string& module,
				   const std::string& name, MethodMetaFn rpc);
		friend class Component;
	private:
		std::string _name;
		Config* _config = NULL;
		State* _state = NULL;
		std::map<std::string,
				 std::map<std::string, vci::Method*>> _methods;
		std::map<std::string,
				 std::map<std::string, vci::MethodMeta*>> _meta_methods;
	};

	class Client;

	class Component {
	public:
		Component(std::string name);
		Component& run();
		Component& wait();
		Component& stop();
		Component& subscribe(const std::string& module,
							 const std::string& notification,
							 Subscriber* subscriber);
		Component& subscribe(const std::string& module,
							 const std::string& notification,
							 SubscriberFn subscriber);
		Component& unsubscribe(const std::string& module,
							   const std::string& notification);
		Component& model(Model& model);
		std::shared_ptr<Client> client();
		~Component();
	private:
		_vci::_CompImpl* _impl;
	};

	class RPCCall {
	public:
		RPCCall();
		~RPCCall();
		EncodedOutput output();
		friend class Client;
	private:
		_vci::_RPCCallImpl* _impl;
	};

	class Subscription {
	public:
		Subscription();
		~Subscription();
		void run();
		void cancel();
		void coalesce();
		void drop_after_limit(uint32_t limit);
		void block_after_limit(uint32_t limit);
		void remove_limit();
		friend class Client;
	private:
		_vci::_SubscriptionImpl* _impl;
	};

	class Client {
	public:
		Client();
		~Client();
		std::shared_ptr<RPCCall> call(
			const std::string& module, const std::string& name,
			const EncodedInput& input);
		void emit(
			const std::string& module, const std::string& name,
			const EncodedInput& data);
		EncodedOutput config_by_model(
			const std::string& model);
		EncodedOutput state_by_model(
			const std::string& model);
		std::shared_ptr<Subscription> subscribe(
			const std::string& module, const std::string& name,
			Subscriber* subscriber);
		std::shared_ptr<Subscription> subscribe(
			const std::string& module, const std::string& name,
			SubscriberFn subscriber);
		friend class Component;
	private:
		Client(_vci::_ClientImpl* impl);
		_vci::_ClientImpl* _impl;
	};
}

#endif
