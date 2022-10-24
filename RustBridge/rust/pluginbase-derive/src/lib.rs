extern crate proc_macro;
extern crate syn;
#[macro_use]
extern crate quote;
use proc_macro::TokenStream;
use syn::{parse_macro_input, ItemStruct};
use quote::quote;
use syn::parse::{ParseStream, Parser, Result};
use syn::{parse, Ident, LitStr, Token};

#[proc_macro_derive(PluginBase)]
pub fn derive_mytrait(input: TokenStream) -> TokenStream {
    let ast = syn::parse(input).unwrap(); 
    impl_my_macro(&ast)
    
}

fn impl_my_macro(ast: &syn::DeriveInput) -> TokenStream {
    let name = &ast.ident;
    let gen = quote! {
        impl PluginBase for #name {
           fn get_supported_methods(&self) -> String {
            json!(Vec::from_iter(self.function_handlers.keys())).to_string()
           }

            fn invoke_method(
                &mut self,
                method: String,
                params: String,
                ctx: &thunder_rs::RequestContext,
            ) -> PluginResponse {
                println!("Incoming method: {:?}", method);
                if let Some(&func) = self.function_handlers.get(&method) {
                    let res: PluginResponse = (func)(self, ctx, params);
                    println!("Invoke Result: {:?}", res);
                    res
                } else {
                    println!("Invoke Result: FAILURE");
                    PluginResponse::Failure {
                        code: -143,
                        message: "Unknown Method".to_owned(),
                    }
                }
            } 
        }
    };
    gen.into()
}

#[cfg(test)]
mod tests {
    #[test]
    fn it_works() {
        let result = 2 + 2;
        assert_eq!(result, 4);
    }
}
