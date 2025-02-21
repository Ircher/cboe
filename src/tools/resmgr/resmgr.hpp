/*
 *  resmgr.h
 *  BoE
 *
 *  Created by Celtic Minstrel on 10-08-24.
 *
 */

#ifndef BOE_RESMGR_H
#define BOE_RESMGR_H

#include <string>
#include <map>
#include <stack>
#include <exception>
#include <memory>
#include <boost/filesystem.hpp>
#include <functional>
#include <iostream>

/// A simple resource manager.
/// Handles loading, retaining, and releasing of resources as necessary.
/// Resources include sounds, images, fonts, and cursors.
///
/// To implement a new resource type, all you have to do is specialize
/// @a ResMgr::resLoader::operator() for the desired resource type.
namespace ResMgr {
	namespace fs = boost::filesystem;
	/// The signature of an ID map function.
	using idMapFn = std::function<std::string(int)>;
	
	/// A resource pool.
	/// @tparam type The type of resource that this pool manages.
	template<typename type> struct resPool {
		/// Get the map of all currently-loaded resources from this resource pool.
		static std::map<std::string,std::shared_ptr<type> >& resources() {
			static std::map<std::string,std::shared_ptr<type> > data;
			return data;
		}
		/// Get the current search path stack for this resource pool.
		static std::stack<fs::path>& resPaths() {
			static std::stack<fs::path> data;
			return data;
		}
		/// Get the current function used to map numerical IDs to string keys (filenames).
		static idMapFn& mapFn() {
			static idMapFn data;
			return data;
		}
		/// Get the map of past path resolutions.
		/// @return A map of relative paths to the absolute path they most recently resolved to.
		static std::map<fs::path,fs::path>& pathFound() {
			static std::map<fs::path,fs::path> data;
			return data;
		}
		/// Convert a relative path to an absolute path by checking the current search path stack.
		/// @param path The path to resolve.
		/// @return The resolved absolute path, or the relative path unchanged if resolution failed.
		static fs::path rel2abs(fs::path path) {
			std::stack<fs::path> tmpPaths = resPaths();
			while(!tmpPaths.empty()) {
				fs::path thisPath = tmpPaths.top()/path;
				if(fs::exists(thisPath)) {
					pathFound()[path] = tmpPaths.top();
					return thisPath;
				}
				tmpPaths.pop();
			}
			// If we got this far, it wasn't found.
			// Just return the original filename unchanged;
			// maybe it can be resolved anyway.
			return path;
		}
	};
	
	/// Handles all the legwork of loading a specific resource.
	/// Must be implemented for each resource you want to manage.
	/// @tparam type The type of resource.
	template<typename type> struct resLoader {
		/// Load a resource of this type from the given file.
		/// @param fname The path to the resource; this will be an absolute path unless resolution failed.
		/// @return A pointer to the loaded resource. The resource manager takes responsibility for freeing it,
		/// so it must be a pointer allocated with `new` rather than `new[]`.
		type* operator() (std::string fname);
	};
	
	/// Thrown if an error occurs while loading a resource.
	class xResMgrErr : public std::exception {
		std::string msg;
	public:
		xResMgrErr() throw() {}
		xResMgrErr(const std::string& str) throw() : msg(str) {}
		~xResMgrErr() throw() {}
		/// @return The error message.
		virtual const char* what() const throw() {
			return msg.c_str();
		}
	};
	
	/// Free a single resource.
	/// @tparam type The type of resource to free.
	/// @param name The key of the resource to free (usually the filename without an extension).
	template<typename type> void free(std::string name) {
		if(resPool<type>::resources().find(name) != resPool<type>::resources().end())
			resPool<type>::resources().erase(name);
	}
	
	/// Free a single resource by numerical ID.
	/// In order for this to work, an ID map function must have first been set with setIdMapFn().
	/// @tparam type The type of resource to free.
	/// @param id The numerical ID of the resource to free.
	/// @throw std::bad_function_call if the ID map function was not set.
	template<typename type> void free(int id) {
		std::string name = resPool<type>::mapFn()(id);
		if(name != "") free<type>(name);
	}
	
	/// Free all resources of a particular type.
	/// @tparam type The type of resource to free.
	template<typename type> void freeAll() {
		resPool<type>::resources().clear();
	}
	
	/// Fetch a single resource, loading it into memory if necessary.
	/// If the resource already exists in memory, it first checks to see if the path resolution has changed,
	/// which could happen if a new path has been pushed on the stack, or a path has been removed.
	/// If it would resolve to a different file than the one currently loaded, the resource is reloaded.
	/// @tparam type The type of the resource to fetch.
	/// @param name The key of the resource to fetch (usually the filename without an extension).
	/// @return A smart pointer to the fetched resource.
	template<typename type> std::shared_ptr<type> get(std::string name) {
		if(resPool<type>::resources().find(name) != resPool<type>::resources().end()) {
			if(resPool<type>::pathFound().find(name) != resPool<type>::pathFound().end()) {
				std::string curPath = resPool<type>::pathFound()[name].string();
				std::string checkPath = resPool<type>::rel2abs(name).string();
				checkPath = checkPath.substr(0,curPath.length());
				if(checkPath != curPath) {
					free<type>(name);
					return get<type>(name);
				}
			}
			return resPool<type>::resources()[name];
		} else {
			type* tmp;
			resLoader<type> load;
			tmp = load(name);
			return resPool<type>::resources()[name] = std::shared_ptr<type>(tmp);
		}
	}
	
	/// Fetch a single resource by numerical ID.
	/// In order for this to work, an ID map function must have first been set with setIdMapFn().
	/// @tparam type The type of the resource to fetch.
	/// @param id The numerical ID of the resource to fetch.
	/// @return A smart pointer to the fetched resource.
	/// @throw xResMgrErr if the ID map function returned an empty string.
	/// @throw std::bad_function_call if the ID map function was not set.
	template<typename type> std::shared_ptr<type> get(int id) {
		std::string name = resPool<type>::mapFn()(id);
		if(name == "") throw xResMgrErr("Invalid resource ID.");
		return get<type>(name);
	}
	
	/// Push a new path onto the path resolution stack
	/// @tparam type The type of resource the path applies to.
	/// @param path The path at which resources of this type may be found.
	template<typename type> void pushPath(fs::path path) {
		resPool<type>::resPaths().push(path);
		if(resPool<type>::resPaths().empty()) std::cerr << "A problem occurred.\n";
	}
	
	/// Pop a path from the path resolution stack.
	/// @tparam type The type of resource the path applies to.
	/// @return The removed path from the top of the stack.
	template<typename type> fs::path popPath() {
		fs::path path = resPool<type>::resPaths().top();
		resPool<type>::resPaths().pop();
		return path;
	}
	
	/// Set an ID map function.
	/// @tparam type The type of resource for which an ID map function should be set.
	/// @param f The new ID map function.
	template<typename type> void setIdMapFn(idMapFn f) {
		resPool<type>::mapFn() = f;
	}
	
	/// Get the ID map function for a resource type.
	/// @tparam type The type of resource to fetch the ID map function for.
	/// @return The currend ID map function for this resource type.
	template<typename type> idMapFn getIdMapFn() {
		return resPool<type>::mapFn();
	}
}

#endif
