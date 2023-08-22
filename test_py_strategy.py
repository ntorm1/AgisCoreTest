import AgisCorePy


class DummyStrategy(AgisCorePy.AgisStrategy):
	def __init__(self, strategy_id: str, portfolio, allocation: float):
		super().__init__(strategy_id, portfolio, allocation)
		self.portfolio = portfolio
		self.strategy_id = strategy_id

	def next(self):
		return

	def reset(self):
		return

	def build(self):
		return