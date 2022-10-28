/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
use serde_json::{json, Value};
use std::collections::HashMap;
use thunder_rs::{PluginResponse, RequestContext, PluginBase, Plugin};
use pluginbase_derive::PluginBase;
#[derive(PluginBase)]
struct Calculator {
    function_handlers:
        HashMap<String, fn(&mut Self, &thunder_rs::RequestContext, String) -> PluginResponse>,
}

impl Calculator {
    fn add2(&mut self, ctx: &thunder_rs::RequestContext, params: String) -> PluginResponse {
        let jparams: Result<Value, serde_json::Error> = serde_json::from_str(params.as_str());
        if let Ok(json_params) = jparams {
            if json_params.is_array() {
                let json_params = json_params.as_array().unwrap();
                let mut sum: u64 = 0;
                for item in json_params {
                    sum += item.as_u64().unwrap();
                }
                PluginResponse::Success(json!(sum))
            } else {
                PluginResponse::Failure {
                    code: -143,
                    message: "Invalid input param".to_owned(),
                }
            }
        } else {
            PluginResponse::Failure {
                code: -143,
                message: "Invalid input param".to_owned(),
            }
        }
    }

    fn mul2(&mut self, ctx: &thunder_rs::RequestContext, params: String) -> PluginResponse {
        println!("Entering add2 function");
        let json_params: Value = serde_json::from_str(params.as_str()).unwrap();
        if json_params.is_array() {
            let json_params = json_params.as_array().unwrap();
            let mut sum: u64 = 0;
            for item in json_params {
                sum *= item.as_u64().unwrap();
            }
            PluginResponse::Success(json!(sum))
        } else {
            PluginResponse::Failure {
                code: -143,
                message: "Invalid input param".to_owned(),
            }
        }
    }

}

impl Plugin for Calculator {
    fn register(&mut self) {
        println!("Registering function handlers");
        self.function_handlers
            .insert("add".to_owned(), Calculator::add2);
        self.function_handlers
            .insert("mul".to_owned(), Calculator::mul2);
    }

    fn on_client_connect(&self, _channel_id: u32) {}

    fn on_client_disconnect(&self, _channel_id: u32) {}
}

fn sample_plugin_init() -> Box<dyn thunder_rs::Plugin> {
    Box::new(Calculator {
        function_handlers: Default::default(),
    })
}

thunder_rs::export_plugin!("Calculator", (1, 0, 0), sample_plugin_init);
