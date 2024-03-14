// Copyright 2023 The Pigweed Authors
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

use pw_format::macros::IntegerDisplayType;

// Used to record calls into the test generator from `generator_test_macro!`.
#[derive(Debug, PartialEq)]
pub enum TestGeneratorOps {
    Finalize,
    StringFragment(String),
    IntegerConversion {
        display_type: IntegerDisplayType,
        type_width: u8,
    },
    StringConversion,
    CharConversion,
    UntypedConversion,
}

// Used to record calls into the test generator from `printf_generator_test_macro!` and friends.
#[derive(Debug, PartialEq)]
pub enum PrintfTestGeneratorOps {
    Finalize,
    StringFragment(String),
    IntegerConversion { ty: String },
    StringConversion,
    CharConversion,
    UntypedConversion,
}

#[cfg(test)]
mod tests {
    use pw_format::macros::IntegerDisplayType;
    use pw_format_test_macros::{
        char_sub_core_fmt_generator_test_macro, char_sub_printf_generator_test_macro,
        core_fmt_generator_test_macro, generator_test_macro,
        integer_sub_core_fmt_generator_test_macro, integer_sub_printf_generator_test_macro,
        printf_generator_test_macro, string_sub_core_fmt_generator_test_macro,
        string_sub_printf_generator_test_macro,
    };

    // Create an alias to ourselves so that the proc macro can name our crate.
    use crate as pw_format_test_macros_test;

    use super::*;

    #[test]
    fn generate_calls_generator_correctly() {
        assert_eq!(
            generator_test_macro!("test %ld %s %c %v", 5, "test", 'c', 1),
            vec![
                TestGeneratorOps::StringFragment("test ".to_string()),
                TestGeneratorOps::IntegerConversion {
                    display_type: IntegerDisplayType::Signed,
                    type_width: 32,
                },
                TestGeneratorOps::StringFragment(" ".to_string()),
                TestGeneratorOps::StringConversion,
                TestGeneratorOps::StringFragment(" ".to_string()),
                TestGeneratorOps::CharConversion,
                TestGeneratorOps::StringFragment(" ".to_string()),
                TestGeneratorOps::UntypedConversion,
                TestGeneratorOps::Finalize
            ]
        );
    }

    #[test]
    fn generate_printf_calls_generator_correctly() {
        assert_eq!(
            printf_generator_test_macro!(
                "test %ld %s %c %v %v",
                5,
                "test",
                'c',
                1 as i32,
                "string" as &str
            ),
            (
                // %ld gets converted to %d because they are equivalent for 32 bit
                // systems.
                // %v gets converted to %d since we pass in a signed integer.
                "test %d %s %c %d %s",
                vec![
                    PrintfTestGeneratorOps::StringFragment("test ".to_string()),
                    PrintfTestGeneratorOps::IntegerConversion {
                        ty: "i32".to_string(),
                    },
                    PrintfTestGeneratorOps::StringFragment(" ".to_string()),
                    PrintfTestGeneratorOps::StringConversion,
                    PrintfTestGeneratorOps::StringFragment(" ".to_string()),
                    PrintfTestGeneratorOps::CharConversion,
                    PrintfTestGeneratorOps::StringFragment(" ".to_string()),
                    PrintfTestGeneratorOps::UntypedConversion,
                    PrintfTestGeneratorOps::StringFragment(" ".to_string()),
                    PrintfTestGeneratorOps::UntypedConversion,
                    PrintfTestGeneratorOps::Finalize
                ]
            )
        );
    }

    // Test that a generator returning an overridden integer conversion specifier
    // changes that and only that conversion specifier in the format string.
    #[test]
    fn generate_printf_substitutes_integer_conversion() {
        assert_eq!(
            integer_sub_printf_generator_test_macro!("test %ld %s %c", 5, "test", 'c'),
            (
                "test %K %s %c",
                vec![
                    PrintfTestGeneratorOps::StringFragment("test ".to_string()),
                    PrintfTestGeneratorOps::IntegerConversion {
                        ty: "i32".to_string(),
                    },
                    PrintfTestGeneratorOps::StringFragment(" ".to_string()),
                    PrintfTestGeneratorOps::StringConversion,
                    PrintfTestGeneratorOps::StringFragment(" ".to_string()),
                    PrintfTestGeneratorOps::CharConversion,
                    PrintfTestGeneratorOps::Finalize
                ]
            )
        );
    }

    // Test that a generator returning an overridden string conversion specifier
    // changes that and only that conversion specifier in the format string.
    #[test]
    fn generate_printf_substitutes_string_conversion() {
        assert_eq!(
            string_sub_printf_generator_test_macro!("test %ld %s %c", 5, "test", 'c'),
            (
                // %ld gets converted to %d because they are equivalent for 32 bit
                // systems.
                "test %d %K %c",
                vec![
                    PrintfTestGeneratorOps::StringFragment("test ".to_string()),
                    PrintfTestGeneratorOps::IntegerConversion {
                        ty: "i32".to_string(),
                    },
                    PrintfTestGeneratorOps::StringFragment(" ".to_string()),
                    PrintfTestGeneratorOps::StringConversion,
                    PrintfTestGeneratorOps::StringFragment(" ".to_string()),
                    PrintfTestGeneratorOps::CharConversion,
                    PrintfTestGeneratorOps::Finalize
                ]
            )
        );
    }

    // Test that a generator returning an overridden character conversion specifier
    // changes that and only that conversion specifier in the format string.
    #[test]
    fn generate_printf_substitutes_char_conversion() {
        assert_eq!(
            char_sub_printf_generator_test_macro!("test %ld %s %c", 5, "test", 'c'),
            (
                // %ld gets converted to %d because they are equivalent for 32 bit
                // systems.
                "test %d %s %K",
                vec![
                    PrintfTestGeneratorOps::StringFragment("test ".to_string()),
                    PrintfTestGeneratorOps::IntegerConversion {
                        ty: "i32".to_string(),
                    },
                    PrintfTestGeneratorOps::StringFragment(" ".to_string()),
                    PrintfTestGeneratorOps::StringConversion,
                    PrintfTestGeneratorOps::StringFragment(" ".to_string()),
                    PrintfTestGeneratorOps::CharConversion,
                    PrintfTestGeneratorOps::Finalize
                ]
            )
        );
    }

    #[test]
    fn generate_core_fmt_calls_generator_correctly() {
        assert_eq!(
            core_fmt_generator_test_macro!("test %ld %s %c %v", 5, "test", 'c', 1),
            (
                "test {} {} {} {}",
                vec![
                    PrintfTestGeneratorOps::StringFragment("test ".to_string()),
                    PrintfTestGeneratorOps::IntegerConversion {
                        ty: "i32".to_string(),
                    },
                    PrintfTestGeneratorOps::StringFragment(" ".to_string()),
                    PrintfTestGeneratorOps::StringConversion,
                    PrintfTestGeneratorOps::StringFragment(" ".to_string()),
                    PrintfTestGeneratorOps::CharConversion,
                    PrintfTestGeneratorOps::StringFragment(" ".to_string()),
                    PrintfTestGeneratorOps::UntypedConversion,
                    PrintfTestGeneratorOps::Finalize
                ]
            )
        );
    }

    // Test that a generator returning an overridden integer conversion specifier
    // changes that and only that conversion specifier in the format string.
    #[test]
    fn generate_core_fmt_substitutes_integer_conversion() {
        assert_eq!(
            integer_sub_core_fmt_generator_test_macro!("test %ld %s %c", 5, "test", 'c'),
            (
                "test {:?} {} {}",
                vec![
                    PrintfTestGeneratorOps::StringFragment("test ".to_string()),
                    PrintfTestGeneratorOps::IntegerConversion {
                        ty: "i32".to_string(),
                    },
                    PrintfTestGeneratorOps::StringFragment(" ".to_string()),
                    PrintfTestGeneratorOps::StringConversion,
                    PrintfTestGeneratorOps::StringFragment(" ".to_string()),
                    PrintfTestGeneratorOps::CharConversion,
                    PrintfTestGeneratorOps::Finalize
                ]
            )
        );
    }

    // Test that a generator returning an overridden string conversion specifier
    // changes that and only that conversion specifier in the format string.
    #[test]
    fn generate_core_fmt_substitutes_string_conversion() {
        assert_eq!(
            string_sub_core_fmt_generator_test_macro!("test %ld %s %c", 5, "test", 'c'),
            (
                "test {} {:?} {}",
                vec![
                    PrintfTestGeneratorOps::StringFragment("test ".to_string()),
                    PrintfTestGeneratorOps::IntegerConversion {
                        ty: "i32".to_string(),
                    },
                    PrintfTestGeneratorOps::StringFragment(" ".to_string()),
                    PrintfTestGeneratorOps::StringConversion,
                    PrintfTestGeneratorOps::StringFragment(" ".to_string()),
                    PrintfTestGeneratorOps::CharConversion,
                    PrintfTestGeneratorOps::Finalize
                ]
            )
        );
    }

    // Test that a generator returning an overridden character conversion specifier
    // changes that and only that conversion specifier in the format string.
    #[test]
    fn generate_core_fmt_substitutes_char_conversion() {
        assert_eq!(
            char_sub_core_fmt_generator_test_macro!("test %ld %s %c", 5, "test", 'c'),
            (
                "test {} {} {:?}",
                vec![
                    PrintfTestGeneratorOps::StringFragment("test ".to_string()),
                    PrintfTestGeneratorOps::IntegerConversion {
                        ty: "i32".to_string(),
                    },
                    PrintfTestGeneratorOps::StringFragment(" ".to_string()),
                    PrintfTestGeneratorOps::StringConversion,
                    PrintfTestGeneratorOps::StringFragment(" ".to_string()),
                    PrintfTestGeneratorOps::CharConversion,
                    PrintfTestGeneratorOps::Finalize
                ]
            )
        );
    }

    #[test]
    fn multiple_format_strings_are_concatenated() {
        assert_eq!(
            generator_test_macro!("a" PW_FMT_CONCAT "b"),
            vec![
                TestGeneratorOps::StringFragment("ab".to_string()),
                TestGeneratorOps::Finalize
            ]
        );
    }
}
