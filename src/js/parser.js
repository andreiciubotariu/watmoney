const AMOUNT_TABLEROW = 'oneweb_balance_information_td_amount';
const AMOUNT_VALUE_BEGIN = '>';
const AMOUNT_VALUE_END = '<';
const INDEX_MEAL_START = 0;
const INDEX_FLEX_START = 3;
const INDEX_FLEX_END = 5;

function parse(html_str, onResult, onError) {
  var search_start = 0;
  var meal_amount = 0;
  var flex_amount = 0;
  for (var x = INDEX_MEAL_START; x <= INDEX_FLEX_END; x++) {
    search_start = html_str.indexOf(AMOUNT_TABLEROW, search_start);
    if (search_start == -1) {
      console.log('Could not find tablerow!');
      onError();
      return;
    }
    var amount_value_begin = html_str.indexOf(AMOUNT_VALUE_BEGIN, search_start);
    var amount_value_end = html_str.indexOf(AMOUNT_VALUE_END, amount_value_begin);
    var amount_str = html_str.substring(amount_value_begin + 1, amount_value_end).trim();
    console.log('Amount str: ' + amount_str)
    var amount = parseFloat(amount_str);
    if (Number.isFinite(amount)) {
      if ((x >= INDEX_MEAL_START) && (x < INDEX_FLEX_START)) {
        meal_amount += amount;
      } else {
        flex_amount += amount;
      }
    }
    search_start = amount_value_end;
  }
  console.log('Meal: ' + meal_amount);
  console.log('Flex: ' + flex_amount);
  var result = {
    meal: meal_amount,
    flex: flex_amount
  };

  onResult(result);
}

module.exports.parse = parse;
